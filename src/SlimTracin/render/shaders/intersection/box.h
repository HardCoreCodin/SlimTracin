#pragma once

#include "../../../core/types.h"
#include "../../../math/vec3.h"
#include "../common.h"

INLINE BoxSide hitBox(RayHit *hit, vec3 *Ro, vec3 *Rd, u8 flags) {
    vec3 octant, RD_rcp = oneOverVec3(*Rd);
    octant.x = signbit(Rd->x) ? 1.0f : -1.0f;
    octant.y = signbit(Rd->y) ? 1.0f : -1.0f;
    octant.z = signbit(Rd->z) ? 1.0f : -1.0f;

    f32 t[6];
    t[0] = ( octant.x - Ro->x) * RD_rcp.x;
    t[1] = ( octant.y - Ro->y) * RD_rcp.y;
    t[2] = ( octant.z - Ro->z) * RD_rcp.z;
    t[3] = (-octant.x - Ro->x) * RD_rcp.x;
    t[4] = (-octant.y - Ro->y) * RD_rcp.y;
    t[5] = (-octant.z - Ro->z) * RD_rcp.z;

    u8 max_axis = t[3] < t[4] ? 3 : 4; if (t[5] < t[max_axis]) max_axis = 5;
    f32 maxt = t[max_axis];
    if (maxt < 0) // Further-away hit is behind the ray - intersection can not occur.
        return NoSide;

    u8 min_axis = t[0] > t[1] ? 0 : 1; if (t[2] > t[min_axis]) min_axis = 2;
    f32 mint = t[min_axis];
    if (maxt < (mint > 0 ? mint : 0))
        return NoSide;

    hit->from_behind = mint < 0; // Further-away hit is in front of the ray, closer one is behind it.
    if (hit->from_behind) {
        mint = maxt;
        min_axis = max_axis;
    }

    BoxSide side = getBoxSide(octant, min_axis);
    hit->position = scaleAddVec3(*Rd, mint, *Ro);
    hit->uv = getUVonUnitCube(hit->position, side);

    if (flags & IS_TRANSPARENT && isTransparentUV(hit->uv)) {
        if (hit->from_behind)
            return NoSide;

        side = getBoxSide(octant, max_axis);
        hit->position = scaleAddVec3(*Rd, maxt, *Ro);
        hit->uv = getUVonUnitCube(hit->position, side);
        if (isTransparentUV(hit->uv))
            return NoSide;

        hit->from_behind = true;
    }

    hit->normal = getVec3Of(0);
    switch (side) {
        case Left:   hit->normal.x = hit->from_behind ? +1.0f : -1.0f; break;
        case Right:  hit->normal.x = hit->from_behind ? -1.0f : +1.0f; break;
        case Bottom: hit->normal.y = hit->from_behind ? +1.0f : -1.0f; break;
        case Top:    hit->normal.y = hit->from_behind ? -1.0f : +1.0f; break;
        case Back:   hit->normal.z = hit->from_behind ? +1.0f : -1.0f; break;
        case Front:  hit->normal.z = hit->from_behind ? -1.0f : +1.0f; break;
        default: return NoSide;
    }

    return side;
}