#pragma once

#include "../../core/types.h"
#include "../../math/vec3.h"

// Filmic Tone-Mapping: https://www.slideshare.net/naughty_dog/lighting-shading-by-john-hable
// ====================
// A = Shoulder Strength (i.e: 0.22)
// B = Linear Strength   (i.e: 0.3)
// C = Linear Angle      (i.e: 0.1)
// D = Toe Strength      (i.e: 0.2)
// E = Toe Numerator     (i.e: 0.01)
// F = Toe Denumerator   (i.e: 0.3)
// LinearWhite = Linear White Point Value (i.e: 11.2)
//   Note: E/F = Toe Angle
//   Note: i.e numbers are NOT gamma corrected(!)
//
// f(x) = ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F
//
// FinalColor = f(LinearColor)/f(LinearWhite)
//
// i.e:
// x = max(0, LinearColor-0.004)
// GammaColor = (x*(6.2*x + 0.5))/(x*(6.2*x+1.7) + 0.06)
//
// A = 6.2
// B = 1.7
//
// C*B = 1/2
// D*F = 0.06
// D*E = 0
//
// C = 1/2*1/B = 1/2*1/1.7 = 1/(2*1.7) = 1/3.4 =
// D = 1
// F = 0.06
// E = 0

//#define TONE_MAP__SHOULDER_STRENGTH 6.2f
//#define TONE_MAP__TOE_STRENGTH 1
//#define TONE_MAP__TOE_NUMERATOR 0
//#define TONE_MAP__TOE_DENOMINATOR 1
//#define TONE_MAP__TOE_ANGLE (TONE_MAP__TOE_NUMERATOR / TONE_MAP__TOE_DENOMINATOR)
//#define TONE_MAP__LINEAR_ANGLE (1.0f/3.4f)
//#define TONE_MAP__LINEAR_WHITE 1
//#define TONE_MAP__LINEAR_STRENGTH 1
// LinearWhite = 1:
// f(LinearWhite) = f(1)
// f(LinearWhite) = (A + C*B + D*E)/(A + B + D*F) - E/F
//#define TONE_MAPPED__LINEAR_WHITE ( \
//    (                               \
//        TONE_MAP__SHOULDER_STRENGTH + \
//        TONE_MAP__LINEAR_ANGLE * TONE_MAP__LINEAR_STRENGTH + \
//        TONE_MAP__TOE_STRENGTH * TONE_MAP__TOE_NUMERATOR \
//    ) / (                           \
//        TONE_MAP__SHOULDER_STRENGTH + TONE_MAP__LINEAR_STRENGTH + \
//        TONE_MAP__TOE_STRENGTH * TONE_MAP__TOE_DENOMINATOR  \
//    ) - TONE_MAP__TOE_ANGLE \
//)

//#ifdef __CUDACC__
//__device__
//__host__
//__forceinline__
//#else
//inline
//#endif
//f32 toneMapped(f32 LinearColor) {
//    f32 x = LinearColor - 0.004f;
//    if (x < 0.0f) x = 0.0f;
//    f32 x2 = x*x;
//    f32 x2_times_sholder_strength = x2 * TONE_MAP__SHOULDER_STRENGTH;
//    f32 x_times_linear_strength   =  x * TONE_MAP__LINEAR_STRENGTH;
//    return (
//                   (
//                           (
//                                   x2_times_sholder_strength + x*x_times_linear_strength + TONE_MAP__TOE_STRENGTH*TONE_MAP__TOE_NUMERATOR
//                           ) / (
//                                   x2_times_sholder_strength +   x_times_linear_strength + TONE_MAP__TOE_STRENGTH*TONE_MAP__TOE_DENOMINATOR
//                           )
//                   ) - TONE_MAP__TOE_ANGLE
//           ) / (TONE_MAPPED__LINEAR_WHITE);
//}

INLINE f32 toneMappedBaked(f32 LinearColor) {
    // x = max(0, LinearColor-0.004)
    // GammaColor = (x*(6.2*x + 0.5))/(x*(6.2*x+1.7) + 0.06)
    // GammaColor = (x*x*6.2 + x*0.5)/(x*x*6.2 + x*1.7 + 0.06)

    f32 x = LinearColor - 0.004f;
    if (x < 0.0f) x = 0.0f;
    f32 x2_times_sholder_strength = x * x * 6.2f;
    return (x2_times_sholder_strength + x*0.5f)/(x2_times_sholder_strength + x*1.7f + 0.06f);
}

INLINE f32 gammaCorrected(f32 x) {
    return (x <= 0.0031308f ? (x * 12.92f) : (1.055f * powf(x, 1.0f/2.4f) - 0.055f));
}

//INLINE f32 gammaCorrectedApproximately(f32 x) {
//    return powf(x, 1.0f/2.2f);
//}

INLINE void setPixelColor(Pixel *pixel, vec3 *color) {
    color->x *= 255;
    color->y *= 255;
    color->z *= 255;
    pixel->color.R = color->x > MAX_COLOR_VALUE ? MAX_COLOR_VALUE : (u8)color->x;
    pixel->color.G = color->y > MAX_COLOR_VALUE ? MAX_COLOR_VALUE : (u8)color->y;
    pixel->color.B = color->z > MAX_COLOR_VALUE ? MAX_COLOR_VALUE : (u8)color->z;
}


//#define setPixelToneMappedColor(pixel, color) \
//        color.x = toneMapped(color.x);    \
//        color.y = toneMapped(color.y);    \
//        color.z = toneMapped(color.z);    \
//        setPixelColor(pixel, color)


INLINE void setPixelBakedToneMappedColor(Pixel *pixel, vec3 *color) {
    color->x = toneMappedBaked(color->x);
    color->y = toneMappedBaked(color->y);
    color->z = toneMappedBaked(color->z);
    setPixelColor(pixel, color);
}

#define setPixelGammaCorrectedColor(pixel, color) \
        color.x = gammaCorrected(color.x);    \
        color.y = gammaCorrected(color.y);    \
        color.z = gammaCorrected(color.z);    \
        setPixelColor(pixel, color)

//#define setPixelApproximatedGammaCorrectedColor(pixel, color) \
//        color.x = gammaCorrectedApproximately(color.x);    \
//        color.y = gammaCorrectedApproximately(color.y);    \
//        color.z = gammaCorrectedApproximately(color.z);    \
//        setPixelColor(pixel, color)


INLINE f32 invDotVec3(vec3 a, vec3 b) {
    return clampValue(-dotVec3(a, b));
}

INLINE f32 schlickFresnel(f32 n1, f32 n2, f32 NdotL) {
    f32 R0 = (n1 - n2) / (n1 + n2);
    return R0 + (1 - R0)*powf(1 - NdotL, 5);
}

INLINE vec3 reflectWithDot(vec3 V, vec3 N, f32 NdotV) {
    return scaleAddVec3(N, -2 * NdotV, V);
}

INLINE vec3 refract(vec3 V, vec3 N, f32 NdotV, f32 n1_over_n2) {
    f32 c = n1_over_n2*n1_over_n2 * (1 - (NdotV*NdotV));
    if (c + EPS > 1) return reflectWithDot(V, N, NdotV);

    return normVec3(
            scaleAddVec3(V, n1_over_n2,
                         scaleVec3(N, n1_over_n2 * -NdotV - sqrtf(1 - c))));
}

INLINE bool isTransparentUV(vec2 uv) {
    u8 v = (u8)(uv.y * 4);
    u8 u = (u8)(uv.x * 4);
    return v % 2 != 0 == u % 2;
}

INLINE BoxSide getBoxSide(vec3 octant, u8 axis) {
    switch (axis) {
        case 0 : return octant.x > 0 ? Right : Left;
        case 3 : return octant.x > 0 ? Left : Right;
        case 1 : return octant.y > 0 ? Top : Bottom;
        case 4 : return octant.y > 0 ? Bottom : Top;
        case 2 : return octant.z > 0 ? Front : Back;
        default: return octant.z > 0 ? Back : Front;
    }
}

INLINE vec2 getUVonUnitCube(vec3 pos, BoxSide side) {
    vec2 uv;

    switch (side) {
        case Top: {
            uv.x = pos.x;
            uv.y = pos.z;
        } break;
        case Bottom: {
            uv.x = -pos.x;
            uv.y = -pos.z;
        } break;
        case Left: {
            uv.x = -pos.z;
            uv.y = pos.y;
        } break;
        case Right:  {
            uv.x = pos.z;
            uv.y = pos.y;
        } break;
        case Front: {
            uv.x = pos.x;
            uv.y = pos.y;
        } break;
        default: {
            uv.x = -pos.x;
            uv.y =  pos.y;
        } break;
    }

    uv.x += 1;
    uv.y += 1;
    uv.x *= 0.5f;
    uv.y *= 0.5f;

    return uv;
}

INLINE void setRayFromCoords(Ray *ray, vec2i coords, Viewport *viewport) {
    ray->origin = viewport->camera->transform.position;
    ray->direction = scaleAddVec3(viewport->projection_plane.right, (f32)coords.x, viewport->projection_plane.start);
    ray->direction = scaleAddVec3(viewport->projection_plane.down,  (f32)coords.y, ray->direction);
    ray->direction = normVec3(ray->direction);
}

INLINE bool hitPlane(vec3 plane_origin, vec3 plane_normal, vec3 *ray_origin, vec3 *ray_direction, RayHit *hit) {
    f32 Rd_dot_n = dotVec3(*ray_direction, plane_normal);
    if (Rd_dot_n == 0) // The ray is parallel to the plane
        return false;

    bool ray_is_facing_the_plane = Rd_dot_n < 0;

    vec3 RtoP = subVec3(*ray_origin, plane_origin);
    f32 Po_to_Ro_dot_n = dotVec3(RtoP, plane_normal);
    hit->from_behind = Po_to_Ro_dot_n < 0;
    if (hit->from_behind == ray_is_facing_the_plane) // The ray can't hit the plane
        return false;

    hit->distance = fabsf(Po_to_Ro_dot_n / Rd_dot_n);
    hit->position = scaleVec3(*ray_direction, hit->distance);
    hit->position = addVec3(*ray_origin,      hit->position);
    hit->normal   = plane_normal;

    return true;
}