#pragma once

#include "../trace.h"
#include "../common.h"

INLINE vec3 shadeSurface(Ray *ray, Trace *trace, Scene *scene, vec3 out_color) {
    vec3 P = trace->closest_hit.position;
    vec3 N = trace->closest_hit.normal;
    vec3 current_color, radiance, RLd, L, H, Rd = ray->direction;
    f32 NdotL, d, d2;
    Material M;
    MaterialSpec mat;
    PointLight *light;

    ray->origin = P;
    M = scene->materials[trace->closest_hit.material_id];

    mat = decodeMaterialSpec(M.uses);
    bool is_ref = mat.has.reflection || mat.has.refraction;
    if (is_ref || mat.uses.phong) RLd = reflectWithDot(Rd, N, -sdotInv(N, Rd));

    current_color = is_ref ? getVec3Of(0) : scene->ambient_light.color;
    for (u32 i = 0; i < scene->settings.point_lights; i++) {
        light = &scene->point_lights[i];
        L = subVec3(light->position_or_direction, P);
        d2 = squaredLengthVec3(L);
        d = sqrtf(d2);
        L = scaleVec3(L, 1.0f / d);

        NdotL = dotVec3(N, L);
        if (NdotL <= 0)
            continue;

        ray->direction = L;
        trace->closest_hit.distance = d;
        trace->closest_hit.distance_squared = d2;
        if (inShadow(ray, trace, scene)) continue;

        if (mat.has.specular) {
            if (mat.uses.blinn) {
                H = normVec3(subVec3(L, Rd));
                radiance = getVec3Of(powf(sdot(N, H), 16.0f));
            } else
                radiance = getVec3Of(powf(sdot(RLd, L), 4.0f));

            if (mat.has.diffuse)
                radiance = scaleAddVec3(M.diffuse, sat(NdotL), radiance);
        } else if (mat.has.diffuse)
            radiance = scaleVec3(M.diffuse, sat(NdotL));

        current_color = mulAddVec3(radiance, scaleVec3(light->color, light->intensity / d2), current_color);
    }

    return addVec3(out_color, current_color);
}

INLINE vec3 shadeSurface2(Ray *ray, Trace *trace, Scene *scene) {
    RayHit *hit = &trace->closest_hit;
    f32 d, d2, NdotL;
    vec3 RLd, Rd = ray->direction;
    vec3 L, N, P, H, radiance;
    Material M;
    MaterialSpec mat;
    PointLight *light;

    vec3 current_color, color = getVec3Of(0), throughput = getVec3Of(1);
    u32 depth = trace->depth;
    while (depth) {
        ray->origin = P = hit->position;
        N = hit->normal;
        M = scene->materials[hit->material_id];

        mat = decodeMaterialSpec(M.uses);
        bool is_ref = mat.has.reflection || mat.has.refraction;
        if (is_ref || mat.uses.phong) RLd = reflectWithDot(Rd, N, -sdotInv(N, Rd));

        current_color = is_ref ? getVec3Of(0) : scene->ambient_light.color;
        for (u32 i = 0; i < scene->settings.point_lights; i++) {
            light = &scene->point_lights[i];
            L = subVec3(light->position_or_direction, P);
            d2 = squaredLengthVec3(L);
            d = sqrtf(d2);
            L = scaleVec3(L, 1.0f / d);

            NdotL = dotVec3(N, L);
            if (NdotL <= 0)
                continue;

            ray->direction = L;
            trace->closest_hit.distance = d;
            trace->closest_hit.distance_squared = d2;
            if (inShadow(ray, trace, scene)) continue;

            if (mat.has.specular) {
                if (mat.uses.blinn) {
                    H = normVec3(subVec3(L, Rd));
                    radiance = getVec3Of(powf(sdot(N, H), 16.0f * M.shininess));
                } else
                    radiance = getVec3Of(powf(sdot(RLd, L), 4.0f * M.shininess));

                if (mat.has.diffuse)
                    radiance = scaleAddVec3(M.diffuse, sat(NdotL), radiance);
            } else if (mat.has.diffuse)
                radiance = scaleVec3(M.diffuse, sat(NdotL));

            current_color = mulAddVec3(radiance, scaleVec3(light->color, light->intensity / d2), current_color);
        }

//        is_specular = M.specular.x || M.specular.y || M.specular.z;
//        current_color = addVec3(M.ambient, M.emission);
//
//        PointLight *light = scene->point_lights;
//        for (u8 l = 0; l < scene->settings.point_lights; l++, light++) {
//            L = light->position_or_direction;
//            if (light->is_directional) {
//                d = d2 = INFINITY;
//            } else {
//                trace->closest_hit.position = L;
//                L = subVec3(L, P);
//                d2 = dotVec3(L, L);
//                d = sqrtf(d2);
//                L = scaleVec3(L, 1.0f / d);
//            }
//
//            NdotI = dotVec3(N, L);
//            if (NdotI <= 0)
//                continue;
//
//            ray->direction = L;
//            hit->distance         = d;
//            hit->distance_squared = d2;
//            if (inShadow(ray, trace, scene))
//                continue;
//
//            Li = light->color;
//            if (!light->is_directional)
//                Li = scaleVec3(Li, 1.0f / (
//                        light->attenuation.x +
//                        light->attenuation.y*d +
//                        light->attenuation.z*d2));
//
//            diff = scaleVec3(M.diffuse, NdotI < 1.0f ? NdotI : 1.0f);
//
//            if (is_specular) {
//                H = normVec3(subVec3(L, Rd));
//                diff = scaleAddVec3(M.specular, powf(DotVec3(N, H), M.shininess), diff);
//            }
//
//            current_color = mulAddVec3(diff, Li, current_color);
//        }

        color = mulAddVec3(current_color, throughput, color);

        if (is_ref && --depth) {
            Rd = reflectVec3(Rd, N);
            ray->origin    = P;
            ray->direction = Rd;
            if (traceRay(ray, trace, scene)) {
                throughput = mulVec3(throughput, M.specular);
                continue;
            }
        }

        break;
    }

    return color;
}