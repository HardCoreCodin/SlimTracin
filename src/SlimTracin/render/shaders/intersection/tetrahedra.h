#pragma once

#include "./quad.h"
#include "../common.h"

#define _0577 0.577350259f
#define _0288 0.288675159f

INLINE bool hitTetrahedron(RayHit *closest_hit, vec3 *Ro, vec3 *Rd, u8 flags) {
    mat3 tangent_matrix;
    vec3 tangent_pos;
    bool found_triangle = false;
    f32 closest_distance = INFINITY;
    RayHit hit;

    for (u8 t = 0; t < 4; t++) {
        tangent_pos = hit.normal = getVec3Of(t == 3 ? _0577 : -_0577);
        switch (t) {
            case 0: hit.normal.y = _0577; break;
            case 1: hit.normal.x = _0577; break;
            case 2: hit.normal.z = _0577; break;
            case 3: tangent_pos.y = -_0577; break;
        }

        if (!hitPlane(tangent_pos, hit.normal, Ro, Rd, &hit))
            continue;

        switch (t) {
            case 0:
                tangent_matrix.X.x = _0577;
                tangent_matrix.X.y = -_0288;
                tangent_matrix.X.z = -_0577;

                tangent_matrix.Y.x = _0288;
                tangent_matrix.Y.y = _0288;
                tangent_matrix.Y.z = _0577;

                tangent_matrix.Z.x = -_0288;
                tangent_matrix.Z.y = _0577;
                tangent_matrix.Z.z = -_0577;
                break;
            case 1:
                tangent_matrix.X.x = _0288;
                tangent_matrix.X.y = _0288;
                tangent_matrix.X.z = _0577;

                tangent_matrix.Y.x = -_0288;
                tangent_matrix.Y.y = _0577;
                tangent_matrix.Y.z = -_0577;

                tangent_matrix.Z.x = _0577;
                tangent_matrix.Z.y = -_0288;
                tangent_matrix.Z.z = -_0577;
                break;
            case 2:
                tangent_matrix.X.x = -_0288;
                tangent_matrix.X.y = _0577;
                tangent_matrix.X.z = -_0577;

                tangent_matrix.Y.x = _0577;
                tangent_matrix.Y.y = -_0288;
                tangent_matrix.Y.z = -_0577;

                tangent_matrix.Z.x = _0288;
                tangent_matrix.Z.y = _0288;
                tangent_matrix.Z.z = _0577;
                break;
            case 3:
                tangent_matrix.X.x = -_0577;
                tangent_matrix.X.y = _0288;
                tangent_matrix.X.z = _0577;

                tangent_matrix.Y.x = _0288;
                tangent_matrix.Y.y = _0288;
                tangent_matrix.Y.z = _0577;

                tangent_matrix.Z.x = _0288;
                tangent_matrix.Z.y = -_0577;
                tangent_matrix.Z.z = _0577;
                break;
        }

        hit.position = scaleAddVec3(*Rd, hit.distance, *Ro);
        tangent_pos = subVec3(hit.position, tangent_pos);
        tangent_pos = mulVec3Mat3(tangent_pos, tangent_matrix);

        if (tangent_pos.x < 0 || tangent_pos.y < 0 || tangent_pos.y + tangent_pos.x > 1)
            continue;

        hit.uv.x = tangent_pos.x;
        hit.uv.y = tangent_pos.y;

        if ((flags & IS_TRANSPARENT) && isTransparentUV(hit.uv))
            continue;

        if (hit.distance < closest_distance) {
            closest_distance = hit.distance;
            hit.area = SQRT3 / 4.0f;
            hit.uv_area = 1;
            hit.NdotV = -dotVec3(hit.normal, *Rd);
            *closest_hit = hit;
            found_triangle = true;
        }
    }

    return found_triangle;
}