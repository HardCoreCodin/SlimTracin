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

    Primitive *hit_primitive, *primitive;
    u32 hit_primitive_id, *primitive_id = primitive_ids;
    for (u32 i = 0; i < primitive_count; i++, primitive_id++) {
        primitive = scene->primitives + *primitive_id;
        if (any_hit && !(primitive->flags & IS_SHADOWING))
            continue;
        if (check_visibility) {
            if (!(primitive->flags & IS_VISIBLE))
                continue;

            if (x <   primitive->screen_bounds.min.x ||
                x >=  primitive->screen_bounds.max.x ||
                y <   primitive->screen_bounds.min.y ||
                y >=  primitive->screen_bounds.max.y)
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
                trace->local_space_ray.direction_reciprocal = oneOverVec3(*Rd);
                trace->closest_mesh_hit.distance = closest_hit->distance == INFINITY ? INFINITY :
                        lengthVec3(subVec3(convertPositionToObjectSpace(closest_hit->position, primitive), *Ro));

                prePrepRay(&trace->local_space_ray);
                current_found = traceMesh(trace, scene->meshes + primitive->id, any_hit);
                if (current_found) *hit = trace->closest_mesh_hit;
                break;
            default:
                continue;
        }

        if (current_found) {
//            if (any_hit && hit->from_behind && scene->materials[primitive->material_id].flags & REFRACTION)
//                continue;

            hit->position       = convertPositionToWorldSpace(hit->position, primitive);
            hit->distance_squared = squaredLengthVec3(subVec3(hit->position, ray->origin));
            if (hit->distance_squared < closest_hit->distance_squared) {
                *closest_hit = *hit;
                hit_primitive_id = *primitive_id;
                hit_primitive = primitive;

                closest_hit->object_type = primitive->type;
                closest_hit->material_id = primitive->material_id;

                found = true;

                if (any_hit)
                    break;
            }
        }
    }

    if (found) {
        if (closest_hit->object_type == PrimitiveType_Mesh) {
            Mesh *mesh = scene->meshes + hit_primitive->id;
            if (mesh->normals_count)
                closest_hit->normal = normVec3(addVec3(addVec3(
                scaleVec3(mesh->triangles[closest_hit->object_id].vertex_normals[2], closest_hit->uv.x),
                scaleVec3(mesh->triangles[closest_hit->object_id].vertex_normals[1], closest_hit->uv.y)),
                scaleVec3(mesh->triangles[closest_hit->object_id].vertex_normals[0],1.0f - (closest_hit->uv.x + closest_hit->uv.y))));
        }
        closest_hit->object_id = hit_primitive_id;
        closest_hit->normal = normVec3(convertDirectionToWorldSpace(closest_hit->normal, hit_primitive));
        closest_hit->distance = sqrtf(closest_hit->distance_squared);
    }

    return found;
}