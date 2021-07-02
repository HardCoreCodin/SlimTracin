#pragma once

#include "../core/types.h"
#include "../core/init.h"
#include "../math/vec3.h"
#include "../scene/box.h"
#include "./shaders/common.h"
#include "./shaders/trace.h"
#include "./shaders/closest_hit/debug.h"
#include "./shaders/closest_hit/surface.h"
#include "./SSB.h"

void setBoxPrimitiveFromAABB(Primitive *box_primitive, AABB *aabb) {
    box_primitive->rotation = getIdentityQuaternion();
    box_primitive->position = scaleVec3(addVec3(aabb->min, aabb->max), 0.5f);
    box_primitive->scale    = scaleVec3(subVec3(aabb->max, aabb->min), 0.5f);
    box_primitive->flags    = IS_TRANSLATED | IS_SCALED | IS_SCALED_NON_UNIFORMLY;
}

void setBoxPrimitiveFromPrimitiveAndAABB(Primitive *box_primitive, Primitive *primitive, AABB *aabb) {
    setBoxPrimitiveFromAABB(box_primitive, aabb);
    box_primitive->scale = mulVec3(box_primitive->scale, primitive->scale);
    box_primitive->rotation = primitive->rotation;
    box_primitive->flags |= IS_ROTATED;
    box_primitive->position = convertPositionToWorldSpace(box_primitive->position, primitive);
}

void drawMeshAccelerationStructure(Viewport *viewport, Mesh *mesh, Primitive *primitive, Primitive *box_primitive) {
    BVHNode *node = mesh->bvh.nodes;
    RGBA color;
    u32 node_count = mesh->bvh.node_count;
    for (u32 node_id = 0; node_id < node_count; node_id++, node++) {
        color = Color(node->primitive_count ? Magenta : (node_id ? Green : Blue));
        setBoxPrimitiveFromPrimitiveAndAABB(box_primitive, primitive, &node->aabb);
        drawBox(viewport, color, &viewport->default_box, box_primitive, BOX__ALL_SIDES);
    }
}

void drawAccelerationStructures(Scene *scene, Viewport *viewport) {
    static Primitive box_primitive;
    BVHNode *node = scene->bvh.nodes;
    Primitive *primitive;
    box_primitive.rotation = getIdentityQuaternion();
    enum ColorID color;
    u32 *primitive_id;

    for (u8 node_id = 0; node_id < scene->bvh.node_count; node_id++, node++) {
        if (node->primitive_count) {
            primitive_id = scene->bvh.leaf_ids + node->first_child_id;
            for (u32 i = 0; i < node->primitive_count; i++, primitive_id++) {
                primitive = &scene->primitives[*primitive_id];
                switch (primitive->type) {
                    case PrimitiveType_Box        : color = Cyan;    break;
                    case PrimitiveType_Quad       : color = White;   break;
                    case PrimitiveType_Sphere     : color = Yellow;  break;
                    case PrimitiveType_Tetrahedron: color = Magenta; break;
                    case PrimitiveType_Mesh       :
                        drawMeshAccelerationStructure(viewport, scene->meshes + primitive->id, primitive, &box_primitive);
                        continue;
                    default:
                        continue;
                }
                drawBox(viewport, Color(color), &viewport->default_box, primitive, BOX__ALL_SIDES);
            }
            color = White;
        } else
            color = node_id ? Grey : Blue;

        setBoxPrimitiveFromAABB(&box_primitive, &node->aabb);
        drawBox(viewport, Color(color), &viewport->default_box, &box_primitive, BOX__ALL_SIDES);
    }
}

void renderOnCPU(Scene *scene, Viewport *viewport) {
    PixelGrid *frame_buffer = viewport->frame_buffer;
    Pixel* pixel = frame_buffer->pixels;
    Dimensions *dim = &frame_buffer->dimensions;

    vec3 ray_origin = viewport->camera->transform.position;
    vec3 start      = viewport->projection_plane.start;
    vec3 right      = viewport->projection_plane.right;
    vec3 down       = viewport->projection_plane.down;
    vec3 color, current = start;
    Pixel black_pixel;
    black_pixel.color = Color(Black);

    Ray ray;
    ray.origin = ray_origin;

    const u16 w = dim->width;
    const u16 h = dim->height;
    enum RenderMode mode = viewport->settings.render_mode;
    Trace *trace = &viewport->trace;

    for (u16 y = 0; y < h; y++) {
        for (u16 x = 0; x < w; x++, pixel++) {
            ray.origin = ray_origin;
            ray.direction = normVec3(current);
            ray.direction_reciprocal = oneOverVec3(ray.direction);
            trace->closest_hit.distance = trace->closest_hit.distance_squared = MAX_DISTANCE;

            if (hitPrimitives(&ray, trace, scene, scene->bvh.leaf_ids, scene->settings.primitives, false, true, x, y)) {
                switch (mode) {
                    case RenderMode_Beauty    : color = shadeSurface(trace, scene, ray.direction, getVec3Of(0)); break;
                    case RenderMode_Depth     : color = shadeDepth(trace->closest_hit.distance);          break;
                    case RenderMode_Normals   : color = shadeDirection(trace->closest_hit.normal);        break;
                    case RenderMode_UVs       : color = shadeUV(trace->closest_hit.uv);                   break;
                }
                if (mode == RenderMode_Beauty)
                    setPixelBakedToneMappedColor(pixel, &color);
                else
                    setPixelColor(pixel, &color);
            } else
                *pixel = black_pixel;

            current = addVec3(current, right);
        }
        current = start = addVec3(start, down);
    }
}