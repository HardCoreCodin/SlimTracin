#pragma once

#include "../core/types.h"
#include "../core/init.h"
#include "../math/vec3.h"
#include "../scene/box.h"

#ifdef USE_DEMO_SCENE
#include "./acceleration_structures/builder_top_down.h"
#endif

#include "./shaders/shade.h"
#include "./SSB.h"
#include "../core/init.h"


void drawMeshAccelerationStructure(Viewport *viewport, Scene *scene, Primitive primitive) {
    Mesh *mesh = &scene->meshes[primitive.id];
    BVHNode *node = mesh->bvh.nodes;
    RGBA color;
    AABB aabb;

    u32 node_count = mesh->bvh.node_count > 200 ? 200 : mesh->bvh.node_count;
    for (u8 node_id = 0; node_id < node_count; node_id++, node++) {
        color = Color(node->primitive_count ? Magenta : (node_id ? Green : Blue));
        primitive.scale = node->aabb.max;
        drawBox(viewport, color, &viewport->default_box, &primitive, BOX__ALL_SIDES);
    }
}

void drawAccelerationStructures(Scene *scene, Viewport *viewport) {
    BVHNode *node = scene->bvh.nodes;
    enum ColorID color;
    Primitive primitive;

    for (u8 node_id = 0; node_id < scene->bvh.node_count; node_id++, node++) {
        if (node->primitive_count) {
            for (u32 i = 0; i < node->primitive_count; i++) {
                primitive = scene->primitives[node->first_child_id];
                switch (primitive.type) {
                    case PrimitiveType_Box        : color = Cyan;    break;
                    case PrimitiveType_Quad       : color = White;   break;
                    case PrimitiveType_Sphere     : color = Yellow;  break;
                    case PrimitiveType_Tetrahedron: color = Magenta; break;
                    case PrimitiveType_Mesh       :
                        drawMeshAccelerationStructure(viewport, scene, primitive);
                        continue;
                    default:
                        continue;
                }

                primitive.scale = getVec3Of(primitive.type == PrimitiveType_Tetrahedron ? SQRT3 / 3 : 1);
                primitive.position = getVec3Of(0);
                drawBox(viewport, Color(color), &viewport->default_box, &primitive, BOX__ALL_SIDES);
            }
            color = White;
        } else
            color = node_id ? Grey : Blue;

        primitive.position = scaleVec3(addVec3(node->aabb.min, node->aabb.max), 0.5f);
        primitive.scale = subVec3(primitive.position, node->aabb.max);
        drawBox(viewport, Color(color), &viewport->default_box, &primitive, BOX__ALL_SIDES);
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
    vec3 current = start;

    Ray ray;
    ray.origin = ray_origin;


    const u16 w = dim->width;
    const u16 h = dim->height;
    enum RenderMode mode = viewport->settings.render_mode;

    for (u16 y = 0; y < h; y++) {
        for (u16 x = 0; x < w; x++, pixel++) {
            ray.origin = ray_origin;
            ray.direction = normVec3(current);
            switch (mode) {
                case RenderMode_Beauty    : renderBeauty( &ray, &viewport->trace, scene, x, y, pixel); break;
                case RenderMode_Depth     : renderDepth(  &ray, &viewport->trace, scene, x, y, pixel); break;
                case RenderMode_Normals   : renderNormals(&ray, &viewport->trace, scene, x, y, pixel); break;
                case RenderMode_UVs       : renderUVs(    &ray, &viewport->trace, scene, x, y, pixel); break;
            }

            current = addVec3(current, right);
        }
        current = start = addVec3(start, down);
    }
}

void onViewportChanged(Scene *scene, Viewport *viewport) {
    xform3 *xform = &viewport->camera->transform;
    Primitive *primitive = scene->primitives;
    vec3 *view_position = scene->ssb.view_positions;

    for (u32 i = 0; i < scene->settings.primitives; i++, primitive++, view_position++)
        *view_position = mulVec3Mat3(
                subVec3(primitive->position, xform->position),
                xform->rotation_matrix_inverted
        );

    updateSceneSSB(scene, viewport);
}

void onRender(Scene *scene, Viewport *viewport) {
    setViewportProjectionPlane(viewport);
    if (viewport->settings.use_GPU) renderOnGPU(scene, viewport);
    else                            renderOnCPU(scene, viewport);
    if (viewport->settings.show_BVH) drawAccelerationStructures(scene, viewport);
    if (viewport->settings.show_SSB) drawSSB(scene, viewport);
}