#pragma once

#include "../trace.h"
#include "../common.h"
#include "../intersection/sphere.h"

INLINE f32 getSphericalVolumeDensity(SphereHit *hit) {
    f32 t1 = hit->t_near > 0 ? hit->t_near : 0;
    f32 t2 = hit->t_far  < hit->furthest ? hit->t_far : hit->furthest;

    // analytical integration of an inverse squared density
    return (hit->c*t1 - hit->b*t1*t1 + t1*t1*t1/3.0f - (
            hit->c*t2 - hit->b*t2*t2 + t2*t2*t2/3.0f)
            ) * (3.0f / 4.0f);
}

INLINE bool shadeLights(Light *lights, u32 light_count, vec3 Ro, vec3 Rd, f32 max_distance, SphereHit *sphere_hit, vec3 *color) {
    Light *light = lights;
    f32 one_over_light_radius, density, distance;
    bool hit_light = false;
    sphere_hit->closest_hit_distance = INFINITY;
    for (u32 i = 0; i < light_count; i++, light++) {
        f32 size = light->intensity * (1.0f / 16.0f);
        one_over_light_radius = 8.0f / size;
        sphere_hit->furthest = max_distance * one_over_light_radius;
        if (hitSphereSimple(Ro, Rd, light->position_or_direction, one_over_light_radius, sphere_hit)) {
            distance = sphere_hit->t_far - size * 8;
            density = powf(getSphericalVolumeDensity(sphere_hit), 8) * 8;
            if (distance < sphere_hit->closest_hit_distance) {
                sphere_hit->closest_hit_distance = distance;
                sphere_hit->closest_hit_density = density;
            }
            *color = scaleAddVec3(light->color, density, *color);

            hit_light = true;
        }
    }
    return hit_light;
}