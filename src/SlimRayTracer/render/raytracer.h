#pragma once

#include "../core/types.h"
#include "../core/init.h"
#include "../math/vec3.h"
#include "../scene/box.h"
#include "./shaders/common.h"
#include "./shaders/trace.h"
#include "./shaders/closest_hit/debug.h"
#include "./shaders/closest_hit/surface.h"
//#include "./shaders/closest_hit/classic.h"
//#include "./shaders/closest_hit/reflection.h"
#include "./SSB.h"
#include "./shaders/closest_hit/fog.h"

void setRenderModeString(enum RenderMode mode, String *string) {
    switch (mode) {
        case RenderMode_Beauty : setString(string, (char*)"Beauty");  break;
        case RenderMode_Normals: setString(string, (char*)"Normals"); break;
        case RenderMode_Depth  : setString(string, (char*)"Depth");   break;
        case RenderMode_UVs    : setString(string, (char*)"UVs");     break;
        default: break;
    }
}

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

void drawBVH(Scene *scene, Viewport *viewport) {
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

void updateScene(Scene *scene, Viewport *viewport) {
    updateSceneBVH(scene, &app->bvh_builder);
    updateSceneSSB(scene, viewport);
}

INLINE void rayTrace(Ray *ray, Trace *trace, Scene *scene, enum RenderMode mode, u16 x, u16 y, Pixel *pixel) {
    vec3 Ro = ray->origin;
    vec3 Rd = ray->direction;
    vec3 color = getVec3Of(0);
    f32 fog, one_over_light_radius, max_distance = INFINITY;
    SphereHit sphere_hit;
    bool hit_found = hitPrimitives(ray, trace, scene, scene->bvh.leaf_ids, scene->settings.primitives, false, true, x, y);
    if (hit_found) {
        max_distance = trace->closest_hit.distance;

        switch (mode) {
            case RenderMode_Beauty: color = shadeSurface(ray, trace, scene); break;
//                material_uses = scene->materials[trace->closest_hit.material_id].flags;
//                if (material_uses & REFLECTION)   color = shadeReflection(ray, trace, scene);
//                else if (material_uses & BLINN)   color = shadeBlinn(     ray, trace, scene);
//                else if (material_uses & PHONG)   color = shadePhong(     ray, trace, scene);
//                else if (material_uses & LAMBERT) color = shadeLambert(   ray, trace, scene);
//                break;
            case RenderMode_Depth  : color = shadeDepth(trace->closest_hit.distance);          break;
            case RenderMode_Normals: color = shadeDirection(trace->closest_hit.normal);        break;
            case RenderMode_UVs    : color = shadeUV(trace->closest_hit.uv);                   break;
        }
    }

    if (mode == RenderMode_Beauty) {
        Light *light = scene->lights;
        if (light) {
            for (u32 i = 0; i < scene->settings.lights; i++, light++) {
                one_over_light_radius = 8.0f / light->intensity;
                if (hitSphereSimple(Ro, Rd, light->position_or_direction, one_over_light_radius,
                                    max_distance * one_over_light_radius,
                                    &sphere_hit)) {
                    fog = computeFog(&sphere_hit, 0, max_distance * one_over_light_radius);
                    fog = powf(fog, 8) * 8;
                    color = scaleAddVec3(light->color, fog, color);
                }
            }
        }

        setPixelBakedToneMappedColor(pixel, &color);
    } else
        setPixelColor(pixel, &color);
}

void renderSceneOnCPU(Scene *scene, Viewport *viewport) {
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
    Trace *trace = &viewport->trace;

    for (u16 y = 0; y < h; y++) {
        for (u16 x = 0; x < w; x++, pixel++) {
            ray.origin = ray_origin;
            ray.direction = normVec3(current);
            ray.direction_reciprocal = oneOverVec3(ray.direction);
            trace->closest_hit.distance = trace->closest_hit.distance_squared = INFINITY;

            rayTrace(&ray, trace, scene, mode, x, y, pixel);

            current = addVec3(current, right);
        }
        current = start = addVec3(start, down);
    }
}

#ifdef __CUDACC__

__global__ void d_render(ProjectionPlane projection_plane, enum RenderMode mode, vec3 camera_position, Trace trace,
                         u16 width,
                         u32 pixel_count,

                         Scene scene,
                         u32        *scene_bvh_leaf_ids,
                         BVHNode    *scene_bvh_nodes,
                         BVHNode    *mesh_bvh_nodes,
                         Mesh       *meshes,
                         Triangle   *mesh_triangles,
                         Light *lights,
                         AreaLight  *area_lights,
                         Material   *materials,
                         Primitive  *primitives,

                         const u32 *mesh_bvh_node_counts,
                         const u32 *mesh_triangle_counts
) {
    u32 i = blockDim.x * blockIdx.x + threadIdx.x;
    if (i >= pixel_count)
        return;

    Pixel *pixel = (Pixel *)&d_pixels[i];

    u16 x = i % width;
    u16 y = i / width;

    Ray ray;
    ray.origin = camera_position;
    ray.direction = normVec3(scaleAddVec3(projection_plane.down, y, scaleAddVec3(projection_plane.right, x, projection_plane.start)));

    scene.lights = lights;
    scene.area_lights  = area_lights;
    scene.materials    = materials;
    scene.primitives   = primitives;
    scene.meshes       = meshes;
    scene.bvh.nodes    = scene_bvh_nodes;
    scene.bvh.leaf_ids = scene_bvh_leaf_ids;

    u32 scene_stack[6], mesh_stack[5];
    trace.mesh_stack  = mesh_stack;
    trace.scene_stack = scene_stack;

    Mesh *mesh = meshes;
    u32 nodes_offset = 0;
    u32 triangles_offset = 0;
    for (u32 m = 0; m < scene.settings.meshes; m++, mesh++) {
        mesh->bvh.node_count = mesh_bvh_node_counts[m];
        mesh->triangle_count = mesh_triangle_counts[m];
        mesh->normals_count  = mesh_triangle_counts[m];
        mesh->triangles      = mesh_triangles + triangles_offset;
        mesh->bvh.nodes      = mesh_bvh_nodes + nodes_offset;

        nodes_offset        += mesh->bvh.node_count;
        triangles_offset    += mesh->triangle_count;
    }

    ray.direction_reciprocal = oneOverVec3(ray.direction);
    trace.closest_hit.distance = trace.closest_hit.distance_squared = INFINITY;

    rayTrace(&ray, &trace, &scene, mode, x, y, pixel);
}

void renderSceneOnGPU(Scene *scene, Viewport *viewport) {
    Dimensions *dim = &viewport->frame_buffer->dimensions;
    u32 pixel_count = dim->width_times_height;
    u16 threads = 256;
    u16 blocks  = pixel_count / threads;
    if (pixel_count < threads) {
        threads = pixel_count;
        blocks = 1;
    } else if (pixel_count % threads)
        blocks++;

    d_render<<<blocks, threads>>>(
            viewport->projection_plane, viewport->settings.render_mode, viewport->camera->transform.position, viewport->trace,

            dim->width,
            pixel_count,

            *scene,
            d_scene_bvh_leaf_ids,
            d_scene_bvh_nodes,
            d_mesh_bvh_nodes,
            d_meshes,
            d_triangles,
            d_lights,
            d_area_lights,
            d_materials,
            d_primitives,

            d_mesh_bvh_node_counts,
            d_mesh_triangle_counts);

    checkErrors()
    downloadN(d_pixels, (u32*)viewport->frame_buffer->pixels, dim->width_times_height)
}

void renderScene(Scene *scene, Viewport *viewport) {
    setViewportProjectionPlane(viewport);
    if (viewport->settings.use_GPU) renderSceneOnGPU(scene, viewport);
    else                            renderSceneOnCPU(scene, viewport);
}
#else
void renderScene(Scene *scene, Viewport *viewport) {
    setViewportProjectionPlane(viewport);
    renderSceneOnCPU(scene, viewport);
}
#endif