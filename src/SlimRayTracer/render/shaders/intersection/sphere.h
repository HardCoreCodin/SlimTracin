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
    f32 t_to_closest = -dotVec3(*Ro, *Rd);
    if (t_to_closest <= 0) { // Ray is aiming away from the sphere
        hit->distance = 0;
        return false;
    }

    hit->distance_squared = squaredLengthVec3(*Ro) - t_to_closest*t_to_closest;
//    if (hit->distance_squared < 0) {
//        hit->distance = 0;
//        return false;
//    }
    f32 delta_squared = 1 - hit->distance_squared;
    if (delta_squared <= 0) { // Ray missed the sphere
        hit->distance = t_to_closest;
        return false;
    }
    // Inside the geometry
    f32 delta = sqrtf(delta_squared);

    hit->distance = t_to_closest - delta;
    bool has_outer_hit = hit->distance > 0;
    if (has_outer_hit) {
        hit->position = scaleAddVec3(*Rd, hit->distance, *Ro);
        hit->uv = getUVonUnitSphere(hit->position);
        if (flags & IS_TRANSPARENT && isTransparentUV(hit->uv))
            has_outer_hit = false;
    }
    if (!has_outer_hit) {
        hit->distance = t_to_closest + delta;
        hit->position = scaleAddVec3(*Rd, hit->distance, *Ro);
        hit->uv = getUVonUnitSphere(hit->position);
        if (flags & IS_TRANSPARENT && isTransparentUV(hit->uv))
            return false;
    }

    hit->from_behind = !has_outer_hit;
    hit->normal = hit->position;

    return true;
}