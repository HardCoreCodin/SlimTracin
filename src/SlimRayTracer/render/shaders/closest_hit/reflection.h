#pragma once

#include "../trace.h"
#include "../common.h"


INLINE vec3 shadeReflection(Ray *ray, Trace *trace, Scene *scene) {
    RayHit *hit = &trace->closest_hit;
    Light *light;
    bool has_diffuse;
    f32 exp, light_distance, light_distance_squared, normal_dot_light_direction;
    vec3 diffuse, specular, hit_position, normal, light_direction, light_radiance, radiance, half_vector,
         current_color, color = getVec3Of(0), throughput = getVec3Of(1), ray_direction = ray->direction;

    u32 depth = trace->depth;
    while (depth) {
        hit_position = ray->origin = hit->position;
        normal = hit->normal;
        exp = 16.0f * scene->materials[hit->material_id].shininess;
        specular    = scene->materials[hit->material_id].specular;
        has_diffuse = scene->materials[hit->material_id].uses & (u8)LAMBERT;
        if (has_diffuse)
            diffuse = scene->materials[hit->material_id].diffuse;

        current_color = getVec3Of(0);
        light = scene->lights;
        for (u32 i = 0; i < scene->settings.lights; i++, light++) {
            if (light->is_directional) {
                light_distance = light_distance_squared = INFINITY;
                light_direction = invertedVec3(light->position_or_direction);
            } else {
                light_direction = subVec3(light->position_or_direction, hit_position);
                light_distance_squared = squaredLengthVec3(light_direction);
                light_distance = sqrtf(light_distance_squared);
                light_direction = scaleVec3(light_direction, 1.0f / light_distance);
            }

            normal_dot_light_direction = dotVec3(normal, light_direction);
            if (normal_dot_light_direction <= 0)
                continue;

            ray->direction = light_direction;
            trace->closest_hit.distance = light_distance;
            trace->closest_hit.distance_squared = light_distance_squared;
            if (!inShadow(ray, trace, scene)) {
                light_radiance = scaleVec3(light->color, light->intensity / light_distance_squared);
                half_vector = subVec3(light_direction, ray_direction);
                radiance = scaleVec3(specular, powf(DotVec3(normal, normVec3(half_vector)), exp));
                if (has_diffuse)
                    radiance = scaleAddVec3(diffuse, clampValue(normal_dot_light_direction), radiance);
                current_color = mulAddVec3(radiance, light_radiance, current_color);
            }
        }

        color = mulAddVec3(current_color, throughput, color);

        if (--depth) {
            ray->direction = ray_direction = reflectVec3(ray_direction, normal);
            ray->origin    = hit_position;
            if (traceRay(ray, trace, scene)) {
                throughput = mulVec3(throughput, specular);
                continue;
            }
        }

        break;
    }

    return color;
}