#pragma once

#include "../../core/base.h"
#include "../../math/vec3.h"
#include "../AABB.h"
#include "./intersection/primitives.h"

INLINE bool traceScene(Ray *ray, Trace *trace, Scene *scene, bool any_hit) {
    ray->direction_reciprocal = oneOverVec3(ray->direction);
    prePrepRay(ray);

    bool hit_left, hit_right, found = false;
    f32 left_distance, right_distance;

    if (!hitAABB(&scene->bvh.nodes->aabb, ray, trace->closest_hit.distance, &left_distance))
        return false;

    if (unlikely(scene->bvh.nodes->child_count))
        return hitPrimitives(ray, trace, scene, scene->bvh.leaf_ids, scene->settings.primitives, any_hit, false, 0, 0);

    BVHNode *left_node = scene->bvh.nodes + scene->bvh.nodes->first_child_id;
    BVHNode *right_node, *tmp_node;
    u32 *stack = trace->scene_stack;
    u32 stack_size = 0;

    while (true) {
        right_node = left_node + 1;

        hit_left  = hitAABB(&left_node->aabb, ray, trace->closest_hit.distance, &left_distance);
        hit_right = hitAABB(&right_node->aabb, ray, trace->closest_hit.distance, &right_distance);

        if (hit_left) {
            if (unlikely(left_node->child_count)) {
                if (hitPrimitives(ray, trace, scene, scene->bvh.leaf_ids + left_node->first_child_id, left_node->child_count, any_hit, false, 0, 0)) {
                    found = true;
                    if (any_hit)
                        break;
                }

                left_node = null;
            }
        } else
            left_node = null;

        if (hit_right) {
            if (unlikely(right_node->child_count)) {
                if (hitPrimitives(ray, trace, scene, scene->bvh.leaf_ids + right_node->first_child_id, right_node->child_count, any_hit, false, 0, 0)) {
                    found = true;
                    if (any_hit)
                        break;
                }
                right_node = null;
            }
        } else
            right_node = null;

        if (left_node) {
            if (right_node) {
                if (!any_hit && left_distance > right_distance) {
                    tmp_node = left_node;
                    left_node = right_node;
                    right_node = tmp_node;
                }
                stack[stack_size++] = right_node->first_child_id;
            }
            left_node = scene->bvh.nodes + left_node->first_child_id;
        } else if (right_node) {
            left_node = scene->bvh.nodes + right_node->first_child_id;
        } else {
            if (stack_size == 0) break;
            left_node = scene->bvh.nodes + stack[--stack_size];
        }
    }

    return found;
}

INLINE bool tracePrimaryRay(Ray *ray, Trace *trace, Scene *scene, u16 x, u16 y) {
    ray->direction_reciprocal = oneOverVec3(ray->direction);
    trace->closest_hit.distance = trace->closest_hit.distance_squared = MAX_DISTANCE;

    return hitPrimitives(ray, trace, scene, scene->bvh.leaf_ids, scene->settings.primitives, false, true, x, y);
}

INLINE bool inShadow(Ray *ray, Trace *trace, Scene *scene) {
    return traceScene(ray, trace, scene, true);
}

INLINE bool traceRay(Ray *ray, Trace *trace, Scene *scene) {
    trace->closest_hit.distance = trace->closest_hit.distance_squared = INFINITY;
    return traceScene(ray, trace, scene, false);
}