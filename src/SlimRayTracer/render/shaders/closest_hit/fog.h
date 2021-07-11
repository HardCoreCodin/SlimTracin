#pragma once

#include "../../../math/vec3.h"
#include "../intersection/sphere.h"

f32 computeFog(SphereHit *hit) {
    // clip integration segment from camera to max_distance
    f32 t1 = hit->t_near > 0  ? hit->t_near : 0;
    f32 t2 = hit->t_far  < hit->furthest ? hit->t_far : hit->furthest;

    // analytical integration of an inverse squared density
    f32 i1 = -(hit->c*t1 - hit->b*t1*t1 + t1*t1*t1/3.0f);
    f32 i2 = -(hit->c*t2 - hit->b*t2*t2 + t2*t2*t2/3.0f);

    return (i2 - i1) * (3.0f / 4.0f);
}