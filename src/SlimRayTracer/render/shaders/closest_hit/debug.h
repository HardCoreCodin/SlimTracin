#pragma once

#include "../trace.h"

INLINE vec3 shadeUV(vec2 uv) {
    vec3 color;

    color.x = 0.5f * uv.x;
    color.y = 0.5f * uv.y;
    color.z = 0.5f;

    return color;
}

INLINE vec3 shadeDirection(vec3 direction) {
    vec3 color;

    color.x = 0.5f * (direction.x + 1);
    color.y = 0.5f * (direction.y + 1);
    color.z = 0.5f * (direction.z + 1);

    return color;
}

INLINE vec3 shadeDepth(f32 distance) {
    return getVec3Of(4 / distance);
}

INLINE vec3 shadeDirectionAndDepth(vec3 direction, f32 distance) {
    f32 factor = 4 / distance;
    vec3 color;

    color.x = factor * (direction.x + 1);
    color.y = factor * (direction.y + 1);
    color.z = factor * (direction.z + 1);

    return color;
}