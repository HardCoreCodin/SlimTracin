#pragma once

#include "../trace.h"
#include "../common.h"


INLINE vec3 shadeLambert(Ray *ray, Trace *trace, Scene *scene) {
    RayHit *hit = &trace->closest_hit;
    vec3 diffuse = scene->materials[hit->material_id].diffuse;
    vec3 hit_position = ray->origin = hit->position;
    vec3 normal = hit->normal;
    vec3 light_direction, light_radiance, radiance;
    f32 light_distance, light_distance_squared, normal_dot_light_direction;

    vec3 color = scene->ambient_light.color;
    Light *light = scene->lights;
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
            radiance = scaleVec3(diffuse, clampValue(normal_dot_light_direction));
            color = mulAddVec3(radiance, light_radiance, color);
        }
    }

    return color;
}

INLINE vec3 shadePhong(Ray *ray, Trace *trace, Scene *scene) {
    RayHit *hit = &trace->closest_hit;
    f32 exp = 4.0f   * scene->materials[hit->material_id].shininess;
    bool has_diffuse = scene->materials[hit->material_id].uses & (u8)LAMBERT;
    vec3 diffuse     = scene->materials[hit->material_id].diffuse;
    vec3 specular    = scene->materials[hit->material_id].specular;
    vec3 hit_position = ray->origin = hit->position;
    vec3 normal = hit->normal;
    vec3 reflected_direction = reflectWithDot(ray->direction, normal, -invDotVec3(normal, ray->direction));
    vec3 light_direction, light_radiance, radiance;
    f32 light_distance, light_distance_squared, normal_dot_light_direction;

    vec3 color = scene->ambient_light.color;
    Light *light = scene->lights;
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
            radiance   = scaleVec3(specular, powf(DotVec3(reflected_direction, light_direction), exp));
            if (has_diffuse)
                radiance = scaleAddVec3(diffuse, clampValue(normal_dot_light_direction), radiance);
            color = mulAddVec3(radiance, light_radiance, color);
        }
    }

    return color;
}

INLINE vec3 shadeBlinn(Ray *ray, Trace *trace, Scene *scene) {
    RayHit *hit = &trace->closest_hit;
    f32 exp = 16.0f  * scene->materials[hit->material_id].shininess;
    bool has_diffuse = scene->materials[hit->material_id].uses & (u8)LAMBERT;
    vec3 diffuse     = scene->materials[hit->material_id].diffuse;
    vec3 specular    = scene->materials[hit->material_id].specular;
    vec3 hit_position = ray->origin = hit->position;
    vec3 ray_direction = ray->direction;
    vec3 normal = hit->normal;
    vec3 light_direction, light_radiance, radiance, half_vector;
    f32 light_distance, light_distance_squared, normal_dot_light_direction;

    vec3 color = scene->ambient_light.color;
    Light *light = scene->lights;
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
            color = mulAddVec3(radiance, light_radiance, color);
        }
    }

    return color;
}