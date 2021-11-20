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
            Triangle *triangle = mesh->triangles + closest_hit->object_id;
            if (mesh->normals_count > 0 || mesh->uvs_count > 0) {
                f32 u = closest_hit->uv.u;
                f32 v = closest_hit->uv.v;
                f32 w = 1 - u - v;
                if (mesh->uvs_count) {
                    closest_hit->uv.x = fast_mul_add(triangle->uvs[2].x, u, fast_mul_add(triangle->uvs[1].u, v, triangle->uvs[0].u * w));
                    closest_hit->uv.y = fast_mul_add(triangle->uvs[2].y, u, fast_mul_add(triangle->uvs[1].v, v, triangle->uvs[0].v * w));
                }
                if (mesh->normals_count) {
                    closest_hit->normal.x = fast_mul_add(triangle->vertex_normals[2].x, u, fast_mul_add(triangle->vertex_normals[1].x, v, triangle->vertex_normals[0].x * w));
                    closest_hit->normal.y = fast_mul_add(triangle->vertex_normals[2].y, u, fast_mul_add(triangle->vertex_normals[1].y, v, triangle->vertex_normals[0].y * w));
                    closest_hit->normal.z = fast_mul_add(triangle->vertex_normals[2].z, u, fast_mul_add(triangle->vertex_normals[1].z, v, triangle->vertex_normals[0].z * w));
                    closest_hit->normal = normVec3(closest_hit->normal);
                }
            }
        }
        closest_hit->object_id = hit_primitive_id;
        closest_hit->normal = normVec3(convertDirectionToWorldSpace(closest_hit->normal, hit_primitive));
        closest_hit->distance = sqrtf(closest_hit->distance_squared);
    }

    return found;
}