#pragma once

#include "../common.h"

INLINE bool hitTriangles(Ray *ray, RayHit *hit, RayHit *closest_hit, Triangle *triangles, u32 triangle_count, bool any_hit, bool use_vertex_normals) {
    vec3 UV;
    bool found_triangle = false;
    Triangle *triangle = triangles;
    for (u32 i = 0; i < triangle_count; i++, triangle++) {
        if (hitPlane(triangle->position, triangle->normal, &ray->origin, &ray->direction, hit) &&
            hit->distance < closest_hit->distance) {

            UV = subVec3(hit->position, triangle->position);
            UV = mulVec3Mat3(UV, triangle->world_to_tangent);
            if (UV.x < 0 || UV.y < 0 || (UV.x + UV.y) > 1)
                continue;

            closest_hit->uv.x = UV.x;
            closest_hit->uv.y = UV.y;
            closest_hit->from_behind = hit->from_behind;
            closest_hit->distance = hit->distance;
            closest_hit->position = hit->position;
            closest_hit->normal = use_vertex_normals ?
                    normVec3(
                    addVec3(
                    addVec3(
                    scaleVec3(triangle->vertex_normals[2], UV.x),
                    scaleVec3(triangle->vertex_normals[1], UV.y)),
                    scaleVec3(triangle->vertex_normals[0],1.0f - (UV.x + UV.y)))
                    ) : hit->normal;

            found_triangle = true;

            if (any_hit)
                break;
        }
    }

    return found_triangle;
}

INLINE bool traceMesh(Trace *trace, Mesh *mesh, bool any_hit) {
    Ray *ray = &trace->local_space_ray;
    RayHit *closest_hit = &trace->closest_mesh_hit;
    RayHit *hit = &trace->current_hit;
    u32 *stack = trace->mesh_stack;

    bool hit_left, hit_right, use_vertex_normals = mesh->normals_count != 0, found = false;
    f32 left_distance, right_distance;

    if (!hitAABB(&mesh->bvh.nodes->aabb, ray, closest_hit->distance, &left_distance))
        return false;

    if (unlikely(mesh->bvh.nodes->primitive_count))
        return hitTriangles(ray, hit, closest_hit, mesh->triangles, mesh->triangle_count, any_hit, use_vertex_normals);

    BVHNode *left_node = mesh->bvh.nodes + mesh->bvh.nodes->first_child_id;
    BVHNode *right_node, *tmp_node;
    u32 stack_size = 0;

    while (true) {
        right_node = left_node + 1;

        hit_left  = hitAABB(&left_node->aabb,  ray, closest_hit->distance, &left_distance);
        hit_right = hitAABB(&right_node->aabb, ray, closest_hit->distance, &right_distance);

        if (hit_left) {
            if (unlikely(left_node->primitive_count)) {
                if (hitTriangles(ray, hit, closest_hit, mesh->triangles + left_node->first_child_id, left_node->primitive_count, any_hit, use_vertex_normals)) {
                    found = true;
                    if (any_hit)
                        break;
                }

                left_node = null;
            }
        } else
            left_node = null;

        if (hit_right) {
            if (unlikely(right_node->primitive_count)) {
                if (hitTriangles(ray, hit, closest_hit, mesh->triangles + right_node->first_child_id, right_node->primitive_count, any_hit, use_vertex_normals)) {
                    found = true;
                    if (any_hit)
                        break;
                }

                right_node = null;
            }
        } else right_node = null;

        if (left_node) {
            if (right_node) {
                if (!any_hit && left_distance > right_distance) {
                    tmp_node = left_node;
                    left_node = right_node;
                    right_node = tmp_node;
                }
                stack[stack_size++] = right_node->first_child_id;
            }
            left_node = mesh->bvh.nodes + left_node->first_child_id;
        } else if (right_node) {
            left_node = mesh->bvh.nodes + right_node->first_child_id;
        } else {
            if (stack_size == 0) break;
            left_node = mesh->bvh.nodes + stack[--stack_size];
        }
    }

    return found;
}