#pragma once

#include "../trace.h"
#include "../common.h"

INLINE vec3 shadeLambert(Scene *scene, SimpleBVHNode *bvh_nodes, Masks *masks, vec3 Rd, vec3 P, vec3 N, vec3 out_color) {
    f32 d, d2;
    vec3 L;
    vec3 light_color, color = scene->ambient_light.color;

    if (dot(N, Rd) > 0) N = invertedVec3(N);

    Ray ray;
    ray.origin = P;
    PointLight *light;
    for (u8 i = 0; i < POINT_LIGHT_COUNT; i++) {
        light = &scene->point_lights[i];
        L = subVec3(light->position, P);

        d2 = squaredLengthVec3(L);
        d = sqrtf(d2);
        L = scaleVec3(L, 1.0f / d);

        ray.direction = L;
        prepRay(&ray);
        if (inShadow(&ray, scene, bvh_nodes, masks, d)) continue;

        light_color = scaleVec3(light->color,light->intensity * sdot(N, L) / d2);
        color = addVec3(color, light_color);
    }

    return addVec3(out_color, color);
}

INLINE vec3 shadePhong(Scene *scene, SimpleBVHNode *bvh_nodes, Masks *masks, vec3 Rd, vec3 P, vec3 N, vec3 out_color) {
    vec3 light_color, color = scene->ambient_light.color;
    vec3 RLd  = reflectWithDot(Rd, N, -sdotInv(N, Rd));
    vec3 L;
    f32 d, d2, li, diff, spec;

    Ray ray;
    ray.origin = P;
    PointLight *light;
    for (u8 i = 0; i < POINT_LIGHT_COUNT; i++) {
        light = &scene->point_lights[i];
        L =subVec3(light->position, P);

        d2 = squaredLengthVec3(L);
        d = sqrtf(d2);
        L = scaleVec3(L, 1.0f / d);

        ray.direction = L;
        prepRay(&ray);
        if (inShadow(&ray, scene, bvh_nodes, masks, d)) continue;

        li = light->intensity / d2;
        diff = li * sdot(N, L);
        spec = li * powf(sdot(RLd, L), 4);

        light_color = scaleVec3(light->color, diff + spec);
        color = addVec3(color, light_color);
    }

    return addVec3(out_color, color);
}

INLINE vec3 shadeBlinn(Scene *scene, SimpleBVHNode *bvh_nodes, Masks *masks, vec3 Rd, vec3 P, vec3 N, vec3 out_color) {
    vec3 light_color, color = scene->ambient_light.color;
    vec3 L, H;
    f32 d, d2, li, diff, spec;

    Ray ray;
    ray.origin = P;
    PointLight *light;
    for (u8 i = 0; i < POINT_LIGHT_COUNT; i++) {
        light = &scene->point_lights[i];
        L = subVec3(light->position, P);

        d2 = squaredLengthVec3(L);
        d = sqrtf(d2);
        L = scaleVec3(L, 1.0f / d);

        ray.direction = L;
        prepRay(&ray);
        if (inShadow(&ray, scene, bvh_nodes, masks, d)) continue;

        H = norm(subVec3(L, Rd));

        li = light->intensity / d2;
        diff = li * sdot(N, L);
        spec = li * powf(sdot(N, H), 16);

        light_color = scaleVec3(light->color, diff + spec);
        color = addVec3(color, light_color);
    }

    return addVec3(out_color, color);
}