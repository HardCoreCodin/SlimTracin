#pragma once

#include "../trace.h"
#include "../common.h"

INLINE vec3 shadeReflection(Trace *trace, Scene *scene, SimpleBVHNode *bvh_nodes, Masks *scene_masks, u8 material_id,
                            vec3 Rd, vec3 P, vec3 N, u8 depth, vec3 out_color) {
    vec3 color, light_color, RLd, L, H;
    f32 NdotRd, d, d2, li, diff, spec;
    Material* material = &scene->materials[material_id];
    MaterialSpec mat = decodeMaterialSpec(material->uses);
    f32 di  = material->roughness;
    f32 si  = material->shininess;
    f32 exp = material->uses & (u8)BLINN ? 16 : 4;

    RayHit *hit = &trace->closest_hit;

    if (mat.uses.phong || mat.has.reflection) {
        NdotRd = -sdotInv(N, Rd);
        RLd = reflectWithDot(Rd, N, NdotRd);
    }

    if (mat.has.reflection) {
        color = getVec3Of(0);
        u8 new_hit_depth = depth + 1;
        if (new_hit_depth < MAX_HIT_DEPTH) {
            Ray ray;
            ray.origin = P;
            ray.direction = RLd;
            traceSecondaryRay(&ray, trace, scene, bvh_nodes, scene_masks);
            color = shadeReflection(trace, scene, bvh_nodes, scene_masks, hit->material_id, RLd, hit->position, hit->normal, new_hit_depth, color);
        }
    } else color = scene->ambient_light.color;

    PointLight *light;
    Ray ray;
    ray.origin = P;
    for (u8 i = 0; i < POINT_LIGHT_COUNT; i++) {
        light = &scene->point_lights[i];
        L = subVec3(light->position, P);

        d2 = squaredLengthVec3(L);
        d = sqrtf(d2);
        L = scaleVec3(L, 1.0f / d);

        ray.direction = L;
        prepRay(&ray);
        if (inShadow(&ray, scene, bvh_nodes, scene_masks, d)) continue;

        if (mat.uses.blinn) H = normVec3(subVec3(L, Rd));

        li = light->intensity / d2;
        diff = mat.has.diffuse  ? (li * di * sdot(N, L)) : 0;
        spec = mat.has.specular ? (li * si * powf(mat.uses.blinn ? sdot(N, H) : sdot(RLd, L), exp)) : 0;

        light_color = scaleVec3(light->color, diff + spec);
        color = addVec3(color, light_color);
    }

    if (mat.has.diffuse) color = mulVec3(color, material->diffuse_color);
    return addVec3(out_color, color);
}