#pragma once

#include "../../../core/types.h"
#include "../../../math/vec3.h"
#include "../common.h"

INLINE vec2 getUVonUnitSphere(vec3 direction) {
    vec2 uv;

    f32 x = direction.x;
    f32 y = direction.y;
    f32 z = direction.z;

    f32 z_over_x = x ? (z / x) : 2;
    f32 y_over_x = x ? (y / x) : 2;
    if (z_over_x <=  1 &&
        z_over_x >= -1 &&
        y_over_x <=  1 &&
        y_over_x >= -1) { // Right or Left
        uv.x = z_over_x;
        uv.y = x > 0 ? y_over_x : -y_over_x;
    } else {
        f32 x_over_z = z ? (x / z) : 2;
        f32 y_over_z = z ? (y / z) : 2;
        if (x_over_z <=  1 &&
            x_over_z >= -1 &&
            y_over_z <=  1 &&
            y_over_z >= -1) { // Front or Back:
            uv.x = -x_over_z;
            uv.y = z > 0 ? y_over_z : -y_over_z;
        } else {
            uv.x = x / (y > 0 ? y : -y);
            uv.y = z / y;
        }
    }

    uv.x += 1;  uv.x /= 2;
    uv.y += 1;  uv.y /= 2;

    return uv;
}

INLINE bool hitSphere(RayHit *hit, vec3 *Ro, vec3 *Rd, u8 flags) {
    f32 t = -dotVec3(*Ro, *Rd);
    if (t <= 0)
        return false;

    hit->distance_squared = 1.0f - squaredLengthVec3(scaleAddVec3(*Rd, t, *Ro));
    if (hit->distance_squared <= 0)
        return false;

    // Inside the geometry
    hit->distance = sqrtf(hit->distance_squared);

    f32 inner_hit_distance = t + hit->distance;
    f32 outer_hit_distance = t - hit->distance;

    bool has_outer_hit = outer_hit_distance > 0;
    bool has_inner_hit = inner_hit_distance > 0;

    if (!(has_inner_hit || has_outer_hit))
        return false;

    if (has_outer_hit) {
        hit->position = scaleAddVec3(*Rd, outer_hit_distance, *Ro);
        hit->uv = getUVonUnitSphere(hit->position);
        if (flags & IS_TRANSPARENT && isTransparentUV(hit->uv))
            has_outer_hit = false;
        else
            has_inner_hit = false;
    }

    if (has_inner_hit) {
        hit->position = scaleAddVec3(*Rd, inner_hit_distance, *Ro);
        hit->uv = getUVonUnitSphere(hit->position);
        if (flags & IS_TRANSPARENT && isTransparentUV(hit->uv))
            return false;
    }

    hit->from_behind = has_inner_hit && !has_outer_hit;
    hit->normal = hit->position;

    return true;
}