#pragma once

#include "../../../core/types.h"
#include "../../../math/vec3.h"
#include "../common.h"

INLINE bool hitQuad(RayHit *hit, vec3 *Ro, vec3 *Rd, u8 flags) {
    hit->distance = 0;

    if (Rd->y == 0) // The ray is parallel to the plane
        return false;

    if (Ro->y == 0) // The ray start in the plane
        return false;

    hit->from_behind = Ro->y < 0;
    if (hit->from_behind == Rd->y < 0) // The ray can't hit the plane
        return false;

    hit->distance = fabsf(Ro->y / Rd->y);
    hit->position = scaleAddVec3(*Rd, hit->distance, *Ro);

    if (hit->position.x < -1 ||
        hit->position.x > +1 ||
        hit->position.z < -1 ||
        hit->position.z > +1) {
        return false;
    }

    hit->uv.x = (hit->position.x + 1.0f) * 0.5f;
    hit->uv.y = (hit->position.z + 1.0f) * 0.5f;

    if (flags & IS_TRANSPARENT && isTransparentUV(hit->uv))
        return false;

    hit->normal.x = 0;
    hit->normal.y = 1;
    hit->normal.z = 0;

    return true;
}

INLINE bool hitEmissiveQuads(Ray *ray, Trace *trace, Scene *scene) {
    bool found = false;
    vec3 *Rd = &trace->local_space_ray.direction;
    vec3 *Ro = &trace->local_space_ray.origin;

    RayHit *hit = &trace->current_hit;
    RayHit *closest_hit = &trace->closest_hit;

    Primitive *primitive = scene->primitives;
    for (u32 i = 0; i < scene->settings.primitives; i++, primitive++) {
        if (!(primitive->type == PrimitiveType_Quad && scene->materials[primitive->material_id].uses & EMISSION))
            continue;

        convertPositionAndDirectionToObjectSpace(ray->origin, ray->direction, primitive, Ro, Rd);
        *Ro = scaleAddVec3(*Rd, TRACE_OFFSET, *Ro);

        if (hitQuad(hit, Ro, Rd, primitive->flags)) {
            hit->position       = convertPositionToWorldSpace(hit->position, primitive);
            hit->distance_squared = squaredLengthVec3(subVec3(hit->position, ray->origin));
            if (hit->distance_squared < closest_hit->distance_squared) {
                *closest_hit = *hit;
                closest_hit->object_type = PrimitiveType_Quad;
                closest_hit->material_id = primitive->material_id;
                closest_hit->object_id = i;

                found = true;
            }
        }
    }

    if (found) {
        closest_hit->normal = normVec3(
                convertDirectionToWorldSpace(closest_hit->normal, scene->primitives + closest_hit->object_id));
        closest_hit->distance = sqrtf(closest_hit->distance_squared);
    }

    return found;
}

//INLINE bool hitAreaLights(Ray *ray, Trace *trace, Scene *scene) {
//    f32 u, v, closest_distance = trace->closest_hit.distance;
//    bool found = false;
//
//    vec3 *Ro = &trace->local_space_ray.origin;
//    vec3 *Rd = &trace->local_space_ray.direction;
//
//    *Rd = ray->direction;
//    *Ro = scaleAddVec3(*Rd, TRACE_OFFSET, ray->origin);
//    vec3 hit_position_in_plane_space;
//
//    RayHit *hit = &trace->current_hit;
////    hit->object_type = PrimitiveType_AreaLight;
//
//    AreaLight *area_light = scene->area_lights;
//    for (u8 i = 0; i < scene->settings.area_lights; i++, area_light++)
//        if (hitPlane(area_light->position, area_light->normal, Ro, Rd, hit) &&
//            hit->distance < closest_distance) {
//
//            hit_position_in_plane_space = subVec3(hit->position, area_light->position);
//            u = dotVec3(area_light->U, hit_position_in_plane_space);
//            if (u < 0 || u > area_light->u_length)
//                continue;
//
//            v = dotVec3(area_light->V, hit_position_in_plane_space);
//            if (v < 0 || v > area_light->v_length)
//                continue;
//
//            found = true;
//            hit->object_id = i;
//            hit->distance += TRACE_OFFSET;
//            hit->distance_squared = hit->distance * hit->distance;
//
//            trace->closest_hit = *hit;
//            closest_distance = hit->distance;
//
////            if (any_hit)
////                break;
//        }
//
//    return found;
//}
//
//INLINE u32 collectAreaLightHits(Ray *ray, Trace *trace, Scene *scene) {
//    f32 u, v;
//    vec3 *Ro = &trace->local_space_ray.origin;
//    vec3 *Rd = &trace->local_space_ray.direction;
//
//    *Rd = ray->direction;
//    *Ro = scaleAddVec3(*Rd, TRACE_OFFSET, ray->origin);
//
//    vec3 hit_position_in_plane_space;
//
//    RayHit *hit = trace->quad_light_hits;
//    AreaLight *quad_light = scene->area_lights;
//
//    u32 quad_light_hit_count = 0;
//
//    for (u32 i = 0; i < scene->settings.area_lights; i++, quad_light++) {
//        if (!hitPlane(quad_light->position, quad_light->normal, Ro, Rd, hit)) continue;
//
//        hit_position_in_plane_space = subVec3(hit->position, quad_light->position);
//        u = dotVec3(quad_light->U, hit_position_in_plane_space); if (u < 0 || u > quad_light->u_length) continue;
//        v = dotVec3(quad_light->V, hit_position_in_plane_space); if (v < 0 || v > quad_light->v_length) continue;
//
//        hit->object_id = i;
////        hit->object_type = PrimitiveType_AreaLight;
//        hit->distance += TRACE_OFFSET;
//        hit->distance_squared = hit->distance * hit->distance;
//        hit->material_id = 0;
//        hit++;
//        quad_light_hit_count++;
//    }
//
//    return quad_light_hit_count;
//}