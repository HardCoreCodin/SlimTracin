#pragma once

#include "../core/types.h"
#include "../math/vec3.h"

INLINE bool hitAABB(AABB *aabb, Ray *ray, f32 closest_distance, f32 *distance) {
    vec3 min_t = {*(&aabb->min.x + ray->octant.x), *(&aabb->min.y + ray->octant.y), *(&aabb->min.z + ray->octant.z)};
    vec3 max_t = {*(&aabb->max.x - ray->octant.x), *(&aabb->max.y - ray->octant.y), *(&aabb->max.z - ray->octant.z)};
    min_t = mulAddVec3(min_t, ray->direction_reciprocal, ray->scaled_origin);
    max_t = mulAddVec3(max_t, ray->direction_reciprocal, ray->scaled_origin);

    max_t.x = max_t.x < max_t.y ? max_t.x : max_t.y;
    max_t.x = max_t.x < max_t.z ? max_t.x : max_t.z;
    max_t.x = max_t.x < closest_distance ? max_t.x : closest_distance;

    min_t.x = min_t.x > min_t.y ? min_t.x : min_t.y;
    min_t.x = min_t.x > min_t.z ? min_t.x : min_t.z;
    min_t.x = min_t.x > 0 ? min_t.x : 0;

    *distance = min_t.x;

    return min_t.x <= max_t.x;
}

INLINE AABB mergeAABBs(AABB lhs, AABB rhs) {
    AABB merged;

    merged.min.x = lhs.min.x < rhs.min.x ? lhs.min.x : rhs.min.x;
    merged.min.y = lhs.min.y < rhs.min.y ? lhs.min.y : rhs.min.y;
    merged.min.z = lhs.min.z < rhs.min.z ? lhs.min.z : rhs.min.z;

    merged.max.x = lhs.max.x > rhs.max.x ? lhs.max.x : rhs.max.x;
    merged.max.y = lhs.max.y > rhs.max.y ? lhs.max.y : rhs.max.y;
    merged.max.z = lhs.max.z > rhs.max.z ? lhs.max.z : rhs.max.z;

    return merged;
}

INLINE f32 getSurfaceAreaOfAABB(AABB aabb) {
    vec3 extents = subVec3(aabb.max, aabb.min);
    return extents.x*extents.y + extents.y*extents.z + extents.z*extents.x;
}

void transformAABB(AABB *aabb, Primitive *primitive) {
    f32 x = aabb->min.x;
    f32 y = aabb->min.y;
    f32 z = aabb->min.z;
    f32 X = aabb->max.x;
    f32 Y = aabb->max.y;
    f32 Z = aabb->max.z;

    vec3 v[8] = {
            {x, y, z},
            {x, y, Z},
            {x, Y, z},
            {x, Y, Z},
            {X, y, z},
            {X, y, Z},
            {X, Y, z},
            {X, Y, Z}
    };

    x = y = z = +INFINITY;
    X = Y = Z = -INFINITY;

    vec3 position;
    for (u8 i = 0; i < 8; i++) {
        position = convertPositionToWorldSpace(v[i], primitive);

        if (position.x > X) X = position.x;
        if (position.y > Y) Y = position.y;
        if (position.z > Z) Z = position.z;
        if (position.x < x) x = position.x;
        if (position.y < y) y = position.y;
        if (position.z < z) z = position.z;
    }

    aabb->min.x = x;
    aabb->min.y = y;
    aabb->min.z = z;
    aabb->max.x = X;
    aabb->max.y = Y;
    aabb->max.z = Z;
}