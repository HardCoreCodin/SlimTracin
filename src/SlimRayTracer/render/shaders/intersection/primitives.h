#pragma once

#include "./tetrahedra.h"
#include "./sphere.h"
#include "./quad.h"
#include "./box.h"
#include "./mesh.h"

INLINE bool hitPrimitives(Ray *ray, Trace *trace, Scene *scene,
                          u32 *primitive_ids,
                          u32 primitive_count,
                          bool any_hit,
                          bool check_visibility,
                          u16 x,
                          u16 y
) {
    bool current_found, found = false;
    vec3 *Rd = &trace->local_space_ray.direction;
    vec3 *Ro = &trace->local_space_ray.origin;

    RayHit *hit = &trace->current_hit;
    RayHit *closest_hit = &trace->closest_hit;

    Rect *bounds;
    Primitive *hit_primitive, *primitive;
    u32 *primitive_id = primitive_ids;
    for (u32 i = 0; i < primitive_count; i++, primitive_id++) {
        primitive = scene->primitives + *primitive_id;
        if (check_visibility) {
            if (!(primitive->flags & IS_VISIBLE))
                continue;

            bounds = scene->ssb.bounds + *primitive_id;
            if (x <   bounds->min.x ||
                x >=  bounds->max.x ||
                y <   bounds->min.y ||
                y >=  bounds->max.y)
                continue;
        }

        convertPositionAndDirectionToObjectSpace(ray->origin, ray->direction, primitive, Ro, Rd);
        *Ro = scaleAddVec3(*Rd, TRACE_OFFSET, *Ro);

        switch (primitive->type) {
            case PrimitiveType_Quad       : current_found = hitQuad(       hit, Ro, Rd, primitive->flags); break;
            case PrimitiveType_Box        : current_found = hitBox(        hit, Ro, Rd, primitive->flags); break;
            case PrimitiveType_Sphere     : current_found = hitSphere(     hit, Ro, Rd, primitive->flags); break;
            case PrimitiveType_Tetrahedron: current_found = hitTetrahedron(hit, Ro, Rd, primitive->flags); break;
            case PrimitiveType_Mesh:
                trace->local_space_ray.direction_reciprocal = (
                        primitive->flags & (u8) (IS_ROTATED | IS_SCALED_NON_UNIFORMLY) ?
                        oneOverVec3(*Rd) :
                        ray->direction_reciprocal);
                trace->closest_mesh_hit.distance = closest_hit->distance != INFINITY ?
                        lengthVec3(subVec3(convertPositionToObjectSpace(closest_hit->position, primitive), *Ro)) :
                        INFINITY;
                prePrepRay(&trace->local_space_ray);
                current_found = traceMesh(trace, scene->meshes + primitive->id, any_hit);
                if (current_found) *hit = trace->closest_mesh_hit;
                break;
            default:
                continue;
        }

        if (current_found) {
            hit->position       = convertPositionToWorldSpace(hit->position, primitive);
            hit->distance_squared = squaredLengthVec3(subVec3(hit->position, ray->origin));
            if (hit->distance_squared < closest_hit->distance_squared) {
                *closest_hit = *hit;
                closest_hit->object_type = primitive->type;
                closest_hit->material_id = primitive->material_id;
                closest_hit->object_id = *primitive_id;

                hit_primitive = primitive;
                found = true;

                if (any_hit)
                    break;
            }
        }
    }

    if (found) {
        closest_hit->distance = sqrtf(closest_hit->distance_squared);
        closest_hit->normal = normVec3(convertDirectionToWorldSpace(closest_hit->normal, hit_primitive));
    }

    return found;
}