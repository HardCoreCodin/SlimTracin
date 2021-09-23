#pragma once

#include "../trace.h"
#include "../common.h"
#include "../intersection/sphere.h"
#include "./lights.h"

INLINE vec3 shadePointOnSurface(Shaded *shaded, f32 NdotL) {
    if (shaded->has.specular) {
        vec3 half_vector, color;
        if (shaded->uses.blinn) {
            half_vector = normVec3(subVec3(shaded->light_direction, shaded->viewing_direction));
            color = getVec3Of(powf(DotVec3(shaded->normal, half_vector), 16.0f * shaded->material->shininess));
        } else
            color = getVec3Of(powf(DotVec3(shaded->reflected_direction, shaded->light_direction), 4.0f * shaded->material->shininess));

        if (shaded->has.diffuse)
            return scaleAddVec3(shaded->material->diffuse, clampValue(NdotL), color);
        else
            return color;
    } else
        return scaleVec3(shaded->material->diffuse, clampValue(NdotL));
}

INLINE vec3 shadeFromLights(Shaded *shaded, Ray *ray, Trace *trace, Scene *scene, vec3 color) {
    RayHit *hit = &trace->closest_hit;
    Light *light = scene->lights;
    f32 light_intensity, NdotL;
    for (u32 i = 0; i < scene->settings.lights; i++, light++) {
        hit->position = light->position_or_direction;
        shaded->light_direction = subVec3(hit->position, shaded->position);
        hit->distance_squared = squaredLengthVec3(shaded->light_direction);
        hit->distance = sqrtf(hit->distance_squared);
        shaded->light_direction = scaleVec3(shaded->light_direction, 1.0f / hit->distance);

        NdotL = dotVec3(shaded->normal, shaded->light_direction);
        if (NdotL <= 0)
            continue;

        light_intensity = light->intensity / hit->distance_squared;

        ray->origin    = shaded->position;
        ray->direction = shaded->light_direction;
        if (inShadow(ray, trace, scene))
            continue;

        color = mulAddVec3(shadePointOnSurface(shaded, NdotL), scaleVec3(light->color, light_intensity), color);
    }

    return color;
}

INLINE bool shadeFromEmissiveQuads(Shaded *shaded, Ray *ray, Trace *trace, Scene *scene, vec3 *color) {
    RayHit *hit = &trace->current_hit;
    Primitive *quad = scene->primitives;
    Material *emissive_material;
    vec3 emissive_quad_normal;
    vec3 *Rd = &trace->local_space_ray.direction;
    vec3 *Ro = &trace->local_space_ray.origin;
    bool found = false;

    for (u32 i = 0; i < scene->settings.primitives; i++, quad++) {
        emissive_material = scene->materials + quad->material_id;
        if (quad->type != PrimitiveType_Quad || !(emissive_material->flags & EMISSION))
            continue;

        convertPositionAndDirectionToObjectSpace(shaded->viewing_origin, shaded->viewing_direction, quad, Ro, Rd);
        if (hitQuad(hit, Ro, Rd, quad->flags)) {
            hit->position = convertPositionToWorldSpace(hit->position, quad);
            hit->distance_squared = squaredLengthVec3(subVec3(hit->position, shaded->viewing_origin));
            if (hit->distance_squared < trace->closest_hit.distance_squared) {
                hit->object_id = i;
                hit->object_type = PrimitiveType_Quad;
                hit->material_id = quad->material_id;
                trace->closest_hit = *hit;
                found = true;
            }
        }

        emissive_quad_normal.x = emissive_quad_normal.z = 0;
        emissive_quad_normal.y = 1;
        emissive_quad_normal = mulVec3Quat(emissive_quad_normal, quad->rotation);
        shaded->light_direction = ray->direction = subVec3(quad->position, shaded->position);
        if (dotVec3(emissive_quad_normal, shaded->light_direction) >= 0)
            continue;

        f32 emission_intensity = dotVec3(shaded->normal, getAreaLightVector(quad, shaded->position, shaded->emissive_quad_vertices));
        if (emission_intensity > 0) {
            bool skip = true;
            for (u8 j = 0; j < 4; j++) {
                if (dotVec3(shaded->normal, subVec3(shaded->emissive_quad_vertices[j], shaded->position)) >= 0) {
                    skip = false;
                    break;
                }
            }
            if (skip)
                continue;

            f32 shaded_light = 1;
            Primitive *shadowing_primitive = scene->primitives;
            for (u32 s = 0; s < scene->settings.primitives; s++, shadowing_primitive++) {
                if (quad == shaded->primitive ||
                    quad == shadowing_primitive ||
                    dotVec3(emissive_quad_normal, subVec3(shadowing_primitive->position, quad->position)) <= 0)
                    continue;

                convertPositionAndDirectionToObjectSpace(shaded->position, shaded->light_direction, shadowing_primitive, Ro, Rd);
                *Ro = scaleAddVec3(*Rd, TRACE_OFFSET, *Ro);

                f32 d = 1;
                if (shadowing_primitive->type == PrimitiveType_Sphere) {
                    if (hitSphere(hit, Ro, Rd, shadowing_primitive->flags))
                        d -= (1.0f - sqrtf(hit->distance_squared)) / (hit->distance * emission_intensity * 3);
                } else if (shadowing_primitive->type == PrimitiveType_Quad) {
                    if (hitQuad(hit, Ro, Rd, shadowing_primitive->flags)) {
                        hit->position.y = 0;
                        hit->position.x = hit->position.x < 0 ? -hit->position.x : hit->position.x;
                        hit->position.z = hit->position.z < 0 ? -hit->position.z : hit->position.z;
                        if (hit->position.x > hit->position.z) {
                            hit->position.y = hit->position.z;
                            hit->position.z = hit->position.x;
                            hit->position.x = hit->position.y;
                            hit->position.y = 0;
                        }
                        d -= (1.0f - hit->position.z) / (hit->distance * emission_intensity);
                    }
                }
                if (d < shaded_light)
                    shaded_light = d;
            }
            if (shaded_light > 0)
                *color = mulAddVec3(shadePointOnSurface(shaded, dotVec3(shaded->normal, shaded->light_direction)),
                                   scaleVec3(emissive_material->emission, emission_intensity * shaded_light),
                                   *color);
        }
    }

    if (found) {
//        quad = scene->primitives + trace->closest_hit.object_id;
//        trace->closest_hit.normal = normVec3(convertDirectionToWorldSpace(trace->closest_hit.normal, quad));
        trace->closest_hit.distance = sqrtf(trace->closest_hit.distance_squared);
    }

    return found;
}

INLINE vec3 shadeSurface(Ray *ray, Trace *trace, Scene *scene, bool *lights_shaded) {
    RayHit *hit = &trace->closest_hit;
    vec3 color = getVec3Of(0);
    if (scene->materials[trace->closest_hit.material_id].flags & EMISSION) {
        if (!hit->from_behind)
            color = scene->materials[trace->closest_hit.material_id].emission;
        return color;
    }

    Shaded shaded;
    shaded.viewing_origin    = ray->origin;
    shaded.viewing_direction = ray->direction;
    shaded.primitive = scene->primitives + hit->object_id;
    shaded.material  = scene->materials  + hit->material_id;
    decodeMaterialSpec(shaded.material->flags, &shaded.has, &shaded.uses);

    f32 NdotRd, ior, max_distance = hit->distance;

    bool scene_has_emissive_quads = false;
    for (u32 i = 0; i < scene->settings.primitives; i++)
        if (scene->primitives[i].type == PrimitiveType_Quad &&
            scene->materials[scene->primitives[i].material_id].flags & EMISSION) {
            scene_has_emissive_quads = true;
            break;
        }

    vec3 current_color, throughput = getVec3Of(1);
    u32 depth = trace->depth;
    while (depth) {
        shaded.position = ray->origin = hit->position;
        shaded.normal = hit->from_behind ? invertedVec3(hit->normal) : hit->normal;
        ior = hit->from_behind ? shaded.material->n2_over_n1 : shaded.material->n1_over_n2;

        bool is_ref = shaded.has.reflection || shaded.has.refraction;
        if (is_ref || shaded.uses.phong) {
            NdotRd = -invDotVec3(shaded.normal, shaded.viewing_direction);
            shaded.reflected_direction = reflectWithDot(shaded.viewing_direction, shaded.normal, NdotRd);
        }

        current_color = is_ref ? getVec3Of(0) : scene->ambient_light.color;
        if (scene->lights)
            current_color = shadeFromLights(&shaded, ray, trace, scene, current_color);

        if (scene_has_emissive_quads) {
            if (shadeFromEmissiveQuads(&shaded, ray, trace, scene, &current_color))
                max_distance = hit->distance;
        }

        color = mulAddVec3(current_color, throughput, color);

        if (scene->lights && shadeLights(scene->lights,
                                        scene->settings.lights,
                                        shaded.viewing_origin,
                                        shaded.viewing_direction,
                                        max_distance,
                                        &trace->sphere_hit,
                                        &color))
            *lights_shaded = true;

        if (is_ref && --depth) {
            if (shaded.has.reflection) {
                ray->direction = shaded.reflected_direction;
            } else {
                ray->direction = refract(shaded.viewing_direction, shaded.normal, ior, NdotRd);
                if (ray->direction.x == 0 &&
                    ray->direction.y == 0 &&
                    ray->direction.z == 0)
                    ray->direction = shaded.reflected_direction;
            }
            if (traceRay(ray, trace, scene)) {
                shaded.primitive = scene->primitives + hit->object_id;
                shaded.material = scene->materials + hit->material_id;
                decodeMaterialSpec(shaded.material->flags, &shaded.has, &shaded.uses);

                if (hit->object_type == PrimitiveType_Quad && shaded.has.emission) {
                    color = hit->from_behind ? getVec3Of(0) : shaded.material->emission;
                    break;
                }
                throughput = mulVec3(throughput, shaded.material->specular);

                shaded.viewing_origin    = ray->origin;
                shaded.viewing_direction = ray->direction;

                continue;
            }
        }

        break;
    }

    return color;
}