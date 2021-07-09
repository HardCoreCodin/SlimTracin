#pragma once

#include "../trace.h"
#include "../common.h"

INLINE f32 computeSoftShadowForEmissiveQuad(Primitive *quad, Primitive *shadowing_primitive, RayHit *hit, vec3 origin, vec3 direction) {
    origin = scaleAddVec3(direction, TRACE_OFFSET, origin);
    origin = convertPositionToObjectSpace(origin, shadowing_primitive);
    vec3 target = convertPositionToObjectSpace(quad->position, shadowing_primitive);
    direction = subVec3(target, origin);
    f32 target_distance = lengthVec3(direction);
    direction = scaleVec3(direction, 1.0f / target_distance);

//    vec3 Ro, Rd;
//    convertPositionAndDirectionToObjectSpace(origin, normVec3(subVec3(quad->position, origin)), shadowing_primitive, &Ro, &Rd);
//    Ro = scaleAddVec3(Rd, TRACE_OFFSET, Ro);
    f32 gap = 0;
    switch (shadowing_primitive->type) {
        case PrimitiveType_Sphere:
            hitSphere(hit, &origin, &direction, shadowing_primitive->flags);
            if (hit->distance && hit->distance < target_distance && hit->distance_squared < 1)
                gap = 10*(1 - hit->distance_squared);
            break;
        case PrimitiveType_Quad:
            if (hitQuad(hit, &origin, &direction, shadowing_primitive->flags) && hit->distance < target_distance) {
                hit->position.x = 1.0f - (hit->position.x < 0 ? -hit->position.x : hit->position.x);
                hit->position.z = 1.0f - (hit->position.z < 0 ? -hit->position.z : hit->position.z);
                gap = 25 * (hit->position.x < hit->position.z ? hit->position.x : hit->position.z);
//                gap = 10 * squaredLengthVec3(hit->position);
//                gap = 10 * lengthVec3(hit->position);
//                gap = 20 * (hit->position.x + hit->position.z);
            }

            break;
        default:
            hit->distance = 0;
    }
    if (hit->distance) {
        return clampValue(hit->distance / gap);
    } else
        return 1;
}

INLINE vec3 shadeSurface(Ray *ray, Trace *trace, Scene *scene) {
    RayHit *hit = &trace->closest_hit;
    Material *EM = scene->materials + trace->closest_hit.material_id;
    if (EM->uses & EMISSION)
        return EM->emission;

    f32 d, d2, NdotL, NdotRd, ior;
    vec3 RLd, Rd = ray->direction;
    vec3 v[4], L, LI, N, P, H, radiance;
    Material M;
    MaterialSpec mat;
    Light *light;
    u32 hit_primitive_id;

    vec3 current_color, color = getVec3Of(0), throughput = getVec3Of(1);
    u32 depth = trace->depth;
    while (depth) {
        M = scene->materials[hit->material_id];
        P = ray->origin = hit->position;
        N = hit->from_behind ? invertedVec3(hit->normal) : hit->normal;
        ior = hit->from_behind ? M.n2_over_n1 : M.n1_over_n2;
        hit_primitive_id = hit->object_id;

        mat = decodeMaterialSpec(M.uses);
        bool is_ref = mat.has.reflection || mat.has.refraction;
        if (is_ref || mat.uses.phong) {
            NdotRd = -invDotVec3(N, Rd);
            RLd = reflectWithDot(Rd, N, NdotRd);
        }

        current_color = is_ref ? getVec3Of(0) : scene->ambient_light.color;
        for (u32 i = 0; i < scene->settings.lights; i++) {
            light = &scene->lights[i];
            L = subVec3(light->position_or_direction, P);
            d2 = squaredLengthVec3(L);
            d = sqrtf(d2);
            L = scaleVec3(L, 1.0f / d);

            NdotL = dotVec3(N, L);
            if (NdotL <= 0)
                continue;

            ray->direction = L;
            hit->distance = d;
            hit->distance_squared = d2;
            if (inShadow(ray, trace, scene))
                continue;

            if (mat.has.specular) {
                if (mat.uses.blinn) {
                    H = normVec3(subVec3(L, Rd));
                    radiance = getVec3Of(powf(DotVec3(N, H), 16.0f * M.shininess));
                } else
                    radiance = getVec3Of(powf(DotVec3(RLd, L), 4.0f * M.shininess));

                if (mat.has.diffuse)
                    radiance = scaleAddVec3(M.diffuse, clampValue(NdotL), radiance);
            } else if (mat.has.diffuse)
                radiance = scaleVec3(M.diffuse, clampValue(NdotL));

            LI = scaleVec3(light->color, light->intensity / d2);
            current_color = mulAddVec3(radiance, LI, current_color);
        }

        Primitive *quad = scene->primitives;
        for (u32 i = 0; i < scene->settings.primitives; i++, quad++) {
            if (quad->type != PrimitiveType_Quad)
                continue;

            EM = scene->materials + quad->material_id;
            if (!(EM->uses & EMISSION))
                continue;

            L.x = L.z = 0;
            L.y = 1;
            L = mulVec3Quat(L, quad->rotation);
            LI = subVec3(quad->position, P);
            if (dotVec3(LI, L) >= 0)
                continue;

            NdotL = dotVec3(N, getAreaLightVector(quad, P, v));
            if (NdotL > 0) {
                bool skip = true;
                for (u8 j = 0; j < 4; j++) {
                    if (dotVec3(N, subVec3(v[j], P)) >= 0) {
                        skip = false;
                        break;
                    }
                }
                if (skip)
                    continue;

                f32 shaded_light = 1;
                Primitive *shadowing_primitive = scene->primitives;
                for (u32 s = 0; s < scene->settings.primitives; s++, shadowing_primitive++) {
                    if (shadowing_primitive == quad || s == hit_primitive_id)
//                        ||
//                        dotVec3(subVec3(quad->position, P),
//                                subVec3(shadowing_primitive->position, P)
//                                ) <= 0)
                        continue;

                    d = computeSoftShadowForEmissiveQuad(quad, shadowing_primitive, hit, P, N);
                    if (d < shaded_light)
                        shaded_light = d;
                }
                NdotL *= shaded_light;
                current_color = mulAddVec3(scaleVec3(M.diffuse, NdotL), EM->emission, current_color);
            }
        }

        color = mulAddVec3(current_color, throughput, color);

        if (is_ref && --depth) {
            if (mat.has.reflection) {
                ray->direction = RLd;
            } else {
                ray->direction = refract(Rd, N, ior, NdotRd);
                if (ray->direction.x == 0 &&
                    ray->direction.y == 0 &&
                    ray->direction.z == 0)
                    ray->direction = RLd;
            }
            if (traceRay(ray, trace, scene)) {
                if (hit->object_type == PrimitiveType_Quad) {
                    quad = scene->primitives + hit->object_id;
                    EM = scene->materials + quad->material_id;
                    if (EM->uses & EMISSION) {
                        if (hit->from_behind)
                            break;

                        NdotL = dotVec3(N, getAreaLightVector(quad, hit->position, v));
                        if (NdotL > 0) {
                            color = mulAddVec3(scaleVec3(M.diffuse, (ONE_OVER_PI * NdotL)), EM->emission, color);
                            break;
                        }
                    }
                }
                throughput = mulVec3(throughput, M.specular);
                Rd = ray->direction;
                continue;
            }
        }

        break;
    }

    return color;
}