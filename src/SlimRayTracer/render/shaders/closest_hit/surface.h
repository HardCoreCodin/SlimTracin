#pragma once

#include "../trace.h"
#include "../common.h"

INLINE vec3 shadeSurface(Ray *ray, Trace *trace, Scene *scene) {
    RayHit *hit = &trace->closest_hit;
    f32 d, d2, NdotL;
    vec3 RLd, Rd = ray->direction;
    vec3 L, N, P, H, radiance;
    Material M;
    MaterialSpec mat;
    Light *light;

    vec3 current_color, color = getVec3Of(0), throughput = getVec3Of(1);
    u32 depth = trace->depth;
    while (depth) {
        ray->origin = P = hit->position;
        N = hit->normal;
        M = scene->materials[hit->material_id];

        mat = decodeMaterialSpec(M.uses);
        bool is_ref = mat.has.reflection || mat.has.refraction;
        if (is_ref || mat.uses.phong) RLd = reflectWithDot(Rd, N, -invDotVec3(N, Rd));

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
            trace->closest_hit.distance = d;
            trace->closest_hit.distance_squared = d2;
            if (inShadow(ray, trace, scene)) continue;

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

            current_color = mulAddVec3(radiance, scaleVec3(light->color, light->intensity / d2), current_color);
        }

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