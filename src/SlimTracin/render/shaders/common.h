#pragma once

#include "../../core/types.h"
#include "../../math/vec3.h"
#include "../../math/vec2.h"

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

#define TONE_MAP__SHOULDER_STRENGTH 6.2f
#define TONE_MAP__TOE_STRENGTH 1
#define TONE_MAP__TOE_NUMERATOR 0
#define TONE_MAP__TOE_DENOMINATOR 1
#define TONE_MAP__TOE_ANGLE (TONE_MAP__TOE_NUMERATOR / TONE_MAP__TOE_DENOMINATOR)
#define TONE_MAP__LINEAR_ANGLE (1.0f/3.4f)
#define TONE_MAP__LINEAR_WHITE 1
#define TONE_MAP__LINEAR_STRENGTH 1
// LinearWhite = 1:
// f(LinearWhite) = f(1)
// f(LinearWhite) = (A + C*B + D*E)/(A + B + D*F) - E/F
#define TONE_MAPPED__LINEAR_WHITE ( \
    (                               \
        TONE_MAP__SHOULDER_STRENGTH + \
        TONE_MAP__LINEAR_ANGLE * TONE_MAP__LINEAR_STRENGTH + \
        TONE_MAP__TOE_STRENGTH * TONE_MAP__TOE_NUMERATOR \
    ) / (                           \
        TONE_MAP__SHOULDER_STRENGTH + TONE_MAP__LINEAR_STRENGTH + \
        TONE_MAP__TOE_STRENGTH * TONE_MAP__TOE_DENOMINATOR  \
    ) - TONE_MAP__TOE_ANGLE \
)

INLINE f32 toneMapped(f32 LinearColor) {
    f32 x = LinearColor - 0.004f;
    if (x < 0.0f) x = 0.0f;
    f32 x2 = x*x;
    f32 x2_times_sholder_strength = x2 * TONE_MAP__SHOULDER_STRENGTH;
    f32 x_times_linear_strength   =  x * TONE_MAP__LINEAR_STRENGTH;
    return (
                   (
                           (
                                   x2_times_sholder_strength + x*x_times_linear_strength + TONE_MAP__TOE_STRENGTH*TONE_MAP__TOE_NUMERATOR
                           ) / (
                                   x2_times_sholder_strength +   x_times_linear_strength + TONE_MAP__TOE_STRENGTH*TONE_MAP__TOE_DENOMINATOR
                           )
                   ) - TONE_MAP__TOE_ANGLE
           ) / (TONE_MAPPED__LINEAR_WHITE);
}


INLINE f32 gammaCorrected(f32 x) {
    return (x <= 0.0031308f ? (x * 12.92f) : (1.055f * powf(x, 1.0f/2.4f) - 0.055f));
}

INLINE f32 gammaCorrectedApproximately(f32 x) {
    return powf(x, 1.0f/2.2f);
}
INLINE f32 toneMappedBaked(f32 LinearColor) {
    // x = max(0, LinearColor-0.004)
    // GammaColor = (x*(6.2*x + 0.5))/(x*(6.2*x+1.7) + 0.06)
    // GammaColor = (x*x*6.2 + x*0.5)/(x*x*6.2 + x*1.7 + 0.06)

    f32 x = LinearColor - 0.004f;
    if (x < 0.0f) x = 0.0f;
    f32 x2_times_sholder_strength = x * x * 6.2f;
    return (x2_times_sholder_strength + x*0.5f)/(x2_times_sholder_strength + x*1.7f + 0.06f);
}

INLINE f32 invDotVec3(vec3 a, vec3 b) {
    return clampValue(-dotVec3(a, b));
}

INLINE vec3 reflectWithDot(vec3 V, vec3 N, f32 NdotV) {
    return scaleAddVec3(N, -2 * NdotV, V);
}

INLINE vec3 refract(vec3 V, vec3 N, f32 n1_over_n2, f32 NdotV) {
    f32 c = n1_over_n2*n1_over_n2 * (1 - (NdotV*NdotV));
    if (c > 1)
        return getVec3Of(0);

    V = scaleVec3(V, n1_over_n2);
    V = scaleAddVec3(N, n1_over_n2 * -NdotV - sqrtf(1 - c), V);
    return normVec3(V);
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
    hit->NdotV = -dotVec3(*ray_direction, plane_normal);
    if (hit->NdotV == 0) // The ray is parallel to the plane
        return false;

    bool ray_is_facing_the_plane = hit->NdotV > 0;
    vec3 RP = subVec3(plane_origin, *ray_origin);
    f32 RPdotN = dotVec3(RP, plane_normal);
    hit->from_behind = RPdotN > 0;
    if (hit->from_behind == ray_is_facing_the_plane) // The ray can't hit the plane
        return false;

    hit->distance = fabsf(RPdotN / hit->NdotV);
    hit->position = scaleVec3(*ray_direction, hit->distance);
    hit->position = addVec3(*ray_origin,      hit->position);
    hit->normal   = plane_normal;

    return true;
}

INLINE vec3 getAreaLightVector(Primitive *primitive, vec3 P, vec3 *v) {
    if (primitive->scale.x == 0 ||
        primitive->scale.z == 0)
        return getVec3Of(0);

    vec3 U = getVec3Of(0);
    vec3 V = getVec3Of(0);
    U.x = primitive->scale.x; if (U.x < 0) U.x = -U.x;
    V.z = primitive->scale.z; if (V.z < 0) V.z = -V.z;
    U = mulVec3Quat(U, primitive->rotation);
    V = mulVec3Quat(V, primitive->rotation);

    v[0] = subVec3(subVec3(primitive->position, U), V);
    v[1] = subVec3(addVec3(primitive->position, U), V);
    v[2] = addVec3(addVec3(primitive->position, U), V);
    v[3] = addVec3(subVec3(primitive->position, U), V);
    vec3 u1n = normVec3(subVec3(v[0], P));
    vec3 u2n = normVec3(subVec3(v[1], P));
    vec3 u3n = normVec3(subVec3(v[2], P));
    vec3 u4n = normVec3(subVec3(v[3], P));
    f32 angle12 = acosf(dotVec3(u1n, u2n));
    f32 angle23 = acosf(dotVec3(u2n, u3n));
    f32 angle34 = acosf(dotVec3(u3n, u4n));
    f32 angle41 = acosf(dotVec3(u4n, u1n));

    vec3 out =         scaleVec3(crossVec3(u1n, u2n), angle12);
    out = addVec3(out, scaleVec3(crossVec3(u2n, u3n), angle23));
    out = addVec3(out, scaleVec3(crossVec3(u3n, u4n), angle34));
    out = addVec3(out, scaleVec3(crossVec3(u4n, u1n), angle41));

    return scaleVec3(out, 0.5f);
}

//#define sin2(cos2) (1.0f - (cos2))
//#define tan2(cos2) (sin2((cos2)) / (cos2))
//INLINE f32 smithShadowingMasking(f32 r2, f32 NdotL, f32 NdotO) {
//    if (NdotL <= 0 ||
//        NdotO <= 0)
//        return 0;
//
//    f32 NLcos2 = NdotL * NdotL;
//    f32 NOcos2 = NdotO * NdotO;
//
//    return 4.0f / (
//            (1.0f + sqrtf(1.0f + r2*tan2(NLcos2))) *
//            (1.0f + sqrtf(1.0f + r2*tan2(NOcos2)))
//    );
//}
//INLINE f32 ggxDterm(f32 r2, f32 NdotH) {
//    f32 cos2a = NdotH * NdotH;
//    f32 tan2a = tan2(cos2a);
//    f32 tan2a_plus_r2 = r2 + tan2a;
//    return r2 / (pi * cos2a*cos2a * tan2a_plus_r2*tan2a_plus_r2);
//}
//INLINE f32 schlickFresnel(f32 R0, f32 HdotV) {
//    // R0 = (n1 - n2) / (n1 + n2);
//    return R0 + (1 - R0)*powf(1 - HdotV, 5);
//}
//INLINE f32 D_GGX(f32 NdotH, f32 roughness) {
//    f32 a = NdotH * roughness;
//    f32 k = roughness / (1.0f - NdotH * NdotH + a * a);
//    return k * k * ONE_OVER_PI;
//}

//INLINE f32 ggxGeomSchlick(f32 roughness, f32 NdotV) {
//    return NdotV / fmaxf((NdotV*(1.0f - roughness) + roughness), EPS);
//}
//f32 GeometrySchlickGGX(f32 NdotV, f32 k) {
//    return NdotV / (NdotV * (1.0f - k) + k);
//}
//f32 GeometrySmith(vec3 N, vec3 V, vec3 L, f32 roughness) {
//    f32 k = roughness + 1.0f;
//    k *= k * 0.125f;
//    f32 NdotV = max(dotVec3(N, V), 0.0);
//    f32 NdotL = max(dotVec3(N, L), 0.0);
//    f32 ggx1 = GeometrySchlickGGX(NdotV, roughness);
//    f32 ggx2 = GeometrySchlickGGX(NdotL, roughness);
//
//    return ggx1 * ggx2;
//}
INLINE f32 ggxNDF(f32 roughness_squared, f32 NdotH) { // Trowbridge-Reitz:
    f32 demon = NdotH * NdotH * (roughness_squared - 1.0f) + 1.0f;
    return ONE_OVER_PI * roughness_squared / (demon * demon);
}
INLINE f32 ggxSmithSchlick(f32 NdotL, f32 NdotV, f32 roughness) {
    f32 k = roughness * 0.5f; // Approximation from Karis (UE4)
//    f32 k = roughness + 1.0f;
//    k *= k * 0.125f;
    f32 one_minus_k = 1.0f - k;
    f32 denom = fast_mul_add(NdotV, one_minus_k, k);
    f32 result = NdotV / fmaxf(denom, EPS);
    denom = fast_mul_add(NdotL, one_minus_k, k);
    return result * NdotL / fmaxf(denom, EPS);
}
INLINE vec3 ggxFresnelSchlick(vec3 reflectivity, f32 LdotH) {
    return scaleAddVec3(oneMinusVec3(reflectivity), powf(1.0f - LdotH, 5.0f), reflectivity);
}
INLINE vec3 shadeClassic(vec3 diffuse, f32 NdotL, vec3 specular, f32 specular_factor, f32 exponent, f32 roughness) {
    f32 shininess = 1.0f - roughness;
    specular_factor = NdotL * powf(specular_factor,exponent * shininess) * shininess;
    return scaleAddVec3(specular, specular_factor, diffuse);
}
INLINE vec3 shadePointOnSurface(Shaded *shaded, f32 NdotL) {
    Material *M = shaded->material;
    vec3 nothing = getVec3Of(0);
    vec3 diffuse = scaleVec3(shaded->albedo, M->roughness * NdotL * ONE_OVER_PI);

    if (M->brdf == BRDF_Lambert) return diffuse;
    if (M->brdf == BRDF_Phong) {
        f32 RdotL = DotVec3(shaded->reflected_direction, shaded->light_direction);
        return RdotL ? shadeClassic(diffuse, NdotL, M->reflectivity, RdotL, 4.0f, M->roughness) : diffuse;
    }

    vec3 N = shaded->normal;
    vec3 L = shaded->light_direction;
    vec3 V = invertedVec3(shaded->viewing_direction);
    vec3 H = normVec3(addVec3(L, V));
    f32 NdotH = DotVec3(N, H);
    if (M->brdf == BRDF_Blinn)
        return NdotH ? shadeClassic(diffuse, NdotL, M->reflectivity, NdotH, 16.0f, M->roughness) : diffuse;

    // Cook-Torrance BRDF:
    vec3 F = nothing;
    diffuse = scaleVec3(shaded->albedo, (1.0f - M->metallic) * NdotL * ONE_OVER_PI);
    f32 LdotH = DotVec3(L, H);
    if (LdotH != 1.0f) {
        F = ggxFresnelSchlick(M->reflectivity, LdotH);
        diffuse = mulVec3(diffuse, oneMinusVec3(F));
    }
    if (!M->roughness && NdotH == 1.0f) return diffuse;

    f32 NdotV = DotVec3(N, V);
    if (!NdotV) return diffuse;

    f32 D = ggxNDF(M->roughness * M->roughness, NdotH);
    f32 G = ggxSmithSchlick(NdotL, NdotV, M->roughness);
    return scaleAddVec3(F, D * G / (4.0f * NdotV), diffuse);
}

INLINE f32 dUVbyRayCone(f32 NdotV, f32 cone_width, f32 area, f32 uv_area) {
    f32 projected_cone_width = cone_width / fabsf(NdotV);
    return sqrtf((projected_cone_width * projected_cone_width) * (uv_area / area));
}