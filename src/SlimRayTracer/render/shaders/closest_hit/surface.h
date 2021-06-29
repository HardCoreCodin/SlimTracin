#pragma once

#include "../trace.h"
#include "../common.h"

INLINE vec3 shadeSurface(Trace *trace, Scene *scene, vec3 Rd, vec3 out_color) {
    vec3 P = trace->closest_hit.position;
    vec3 N = trace->closest_hit.normal;
    vec3 light_color, RLd, L, H;
    f32 NdotRd, d, d2, li, diff, spec;
    Material* material = &scene->materials[trace->closest_hit.material_id];
    MaterialSpec mat = decodeMaterialSpec(material->uses);
    f32 di  = material->roughness;
    f32 si  = material->shininess;
    f32 exp = material->uses & (u8)BLINN ? 16.0f : 4.0f;

    bool is_ref = (mat.has.reflection ||
                   mat.has.refraction);
    if (is_ref || mat.uses.phong) {
        NdotRd = -sdotInv(N, Rd);
        RLd = reflectWithDot(Rd, N, NdotRd);
    }
    vec3 color = is_ref ? getVec3Of(0) : scene->ambient_light.color;

    Ray ray;
    ray.origin = P;
    PointLight *light;
    for (u32 i = 0; i < scene->settings.point_lights; i++) {
        light = &scene->point_lights[i];
        L = subVec3(light->position_or_direction, P);

        d2 = squaredLengthVec3(L);
        d = sqrtf(d2);
        L = scaleVec3(L, 1.0f / d);

        ray.direction = L;
        trace->closest_hit.distance = d;
        trace->closest_hit.distance_squared = d2;
        if (inShadow(&ray, trace, scene)) continue;

        if (mat.uses.blinn) H = normVec3(subVec3(L, Rd));

        li = light->intensity / d2;
        diff = mat.has.diffuse  ? (li * di * sdot(N, L)) : 0;
        spec = mat.has.specular ? (li * si * powf(mat.uses.blinn ? sdot(N, H) : sdot(RLd, L), exp)) : 0;

        light_color = scaleVec3(light->color, diff + spec);
        color = addVec3(color, light_color);
    }

    if (mat.has.diffuse) color = mulVec3(color, material->diffuse);
    return addVec3(out_color, color);
}

INLINE vec3 shadeSurface2(Ray *ray, Trace *trace, Scene *scene, u32 depth) {
    RayHit *hit = &trace->closest_hit;
    f32 d, d2, NdotI;
    vec3 Rd = ray->direction;
    vec3 L, Li, diff, N, P, H;
    bool is_specular;
    Material M;

    vec3 current_color, color = getVec3Of(0), throughput = getVec3Of(1);
    while (depth) {
        P = hit->position;
        N = hit->normal;
        M = scene->materials[hit->material_id];
        is_specular = M.specular.x || M.specular.y || M.specular.z;
        current_color = addVec3(M.ambient, M.emission);

        PointLight *light = scene->point_lights;
        for (u8 l = 0; l < scene->settings.point_lights; l++, light++) {
            L = light->position_or_direction;
            if (light->is_directional) {
                d = d2 = INFINITY;
            } else {
                trace->closest_hit.position = L;
                L = subVec3(L, P);
                d2 = dotVec3(L, L);
                d = sqrtf(d2);
                L = scaleVec3(L, 1.0f / d);
            }

            NdotI = dotVec3(N, L);
            if (NdotI <= 0)
                continue;

            ray->origin    = P;
            ray->direction = L;
            hit->distance         = d;
            hit->distance_squared = d2;
            if (inShadow2(ray, trace, scene))
                continue;

            Li = light->color;
            if (!light->is_directional)
                Li = scaleVec3(Li, 1.0f / (
                        light->attenuation.x +
                        light->attenuation.y*d +
                        light->attenuation.z*d2));

            diff = scaleVec3(M.diffuse, NdotI < 1.0f ? NdotI : 1.0f);

            if (is_specular) {
                H = normVec3(subVec3(L, Rd));
                diff = scaleAddVec3(M.specular, powf(DotVec3(N, H), M.shininess), diff);
            }

            current_color = mulAddVec3(diff, Li, current_color);
        }

        color = mulAddVec3(current_color, throughput, color);

        if (is_specular && --depth) {
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