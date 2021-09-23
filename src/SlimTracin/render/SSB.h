#pragma once

#include "../shapes/line.h"

INLINE bool computeSSB(Rect *bounds, vec3 *pos, f32 r, f32 focal_length, Dimensions *dimensions) {
/*
 h = y - t
 HH = zz + tt

 r/z = h/H
 rH = zh
 rrHH = zzhh
 rr(zz + tt) = zz(y -t)(y - t)
 rrzz + rrtt = zz(yy -2ty + tt)
 rrzz + rrtt = zzyy -2tyzz + ttzz
 rrtt - zztt + (2yzz)t + rrzz - zzyy = 0
 (rr - zz)tt + (2yzz)t + zz(rr - yy) = 0

 a = rr - zz
 b = 2yzz
 c = zz(rr - yy)

 t = -b/2a +/- sqrt(bb - 4ac)/2a
 t = -2yzz/2(rr - zz) +/- sqrt(2yzz2yzz - 4(rr - zz)zz(rr - yy))/2(rr - zz)
 t = -yzz/(rr - zz) +/- sqrt(4yzzyzz - 4(rr - zz)zz(rr - yy))/2(rr - zz)
 t = -yzz/(rr - zz) +/- sqrt(yyzzzz - zz(rr - zz)(rr - yy))/(rr - zz)
 t = -yzz/(rr - zz) +/- sqrt(zz(yyzz - (rr - zz)(rr - yy)))/(rr - zz)
 t = -yzz/(rr - zz) +/- z*sqrt(yyzz - (rr - zz)(rr - yy))/(rr - zz)
 t = -yzz/(rr - zz) +/- z*sqrt(yyzz - (rr - zz)(rr - yy))/(rr - zz)

 t/z = 1/(rr - zz) * (-yz +/- sqrt(yyzz - (rr - zz)(rr - yy)))
 t/z = 1/(rr - zz) * (-yz +/- sqrt(yyzz - rr*rr + zz*rr + rr*yy - zz*yy))
 t/z = 1/(rr - zz) * (-yz +/- sqrt(yyzz - zz*yy - rr*rr + zz*rr + rr*yy))
 t/z = 1/(rr - zz) * (-yz +/- sqrt(0 - rr*rr + zz*rr + rr*yy))
 t/z = 1/(rr - zz) * (-yz +/- sqrt(rr(yy - rr + zz))
 t/z = -1/(rr - zz) * -(-yz +/- r*sqrt(zz + yy - rr)
 t/z = 1/(zz - rr) * (yz -/+ r*sqrt(yy + zz - rr)

  t/z = 1/(zz - rr) * (yz -/+ r*sqrt(yy + zz - rr)
  den = zz - rr
  sqr = r * sqrt(yy + den)

  t/z = 1/den * (yz -/+ sqr)
  s/fl = t/z
  s = fl*t/z
  s = fl/den * (yz -/+ sqr)

  f = fl/den
  s = f(yx -/+ sqr)

  s1 = f(yz - sqr)
  s2 = f(yz + sqr)
*/
    bounds->min.x = dimensions->width + 1;
    bounds->max.x = dimensions->width + 1;
    bounds->min.y = dimensions->height + 1;
    bounds->max.y = dimensions->height + 1;

    if (pos->z <= -r)
        return false;

    f32 x = pos->x;
    f32 y = pos->y;
    f32 z = pos->z;
    f32 left, right, top, bottom;
    f32 factor;

    if (z <= r) { // Camera is within the bounds of the shape - fallback to doing weak-projection:
        // The focal length is defined with respect to a 'conceptual' projection-plane of width 2 (spanning in [-1, +1])
        // which is situated at that value's distance away from the camera (along it's view-space's positive Z axis).
        // So half of it's width is 1 while half of it's height is the inverse of the aspect ratio (height / width).

        // w : Half of the width of a projection-plane placed at 'pos' and facing the camera:
        // fl / 1 = abs(z) / w
        // fl * w = abs(z)
        // w = abs(z) / fl
        f32 w = fabsf(z) / focal_length;
        right  = x + r;
        left   = x - r;
        if (w < left || right < -w) // The geometry is out of view horizontally (either on the left or the right)
            return false;

        // h : Half of the height of a projection-plane placed at 'pos' and facing the camera:
        // h / abs(z) = (height/width) / fl
        // h = abs(z) * (height/width) / fl
        // h = abs(z) / fl * (height/width)
        // h = w * (height/width)
        f32 h = w * dimensions->height_over_width;
        top    = y + r;
        bottom = y - r;

        if (h < bottom || top < -h) // The geometry is out of view vertically (either above or below)
            return false;

        bounds->min.x = -w < left   ? (u16)(dimensions->f_width  * (left   + w) / (2 * w)) : 0;
        bounds->max.x = right < w   ? (u16)(dimensions->f_width  * (right  + w) / (2 * w)) : dimensions->width;
        bounds->min.y = top < h     ? (u16)(dimensions->f_height * (h    - top) / (2 * h)) : 0;
        bounds->max.y = -h < bottom ? (u16)(dimensions->f_height * (h - bottom) / (2 * h)) : dimensions->height;

        return true;
    }

    f32 den = z*z - r*r;
    factor = focal_length / den;

    f32 xz = x * z;
    f32 sqr = x*x + den;
    sqr = r * sqrtf(sqr);

    left  = factor*(xz - sqr);
    right = factor*(xz + sqr);
    if (left < 1 && right > -1) {
        factor *= dimensions->width_over_height;

        f32 yz = y * z;
        sqr = r * sqrtf(y*y + den);
        bottom = factor*(yz - sqr);
        top    = factor*(yz + sqr);
        if (bottom < 1 && top > -1) {
            bottom = bottom > -1 ? bottom : -1; bottom += 1;
            top    = top < 1 ? top : 1; top    += 1;
            left   = left > -1 ? left : -1; left   += 1;
            right  = right < 1 ? right : 1; right  += 1;

            top    = 2 - top;
            bottom = 2 - bottom;

            bounds->min.x = (u16)(dimensions->h_width * left);
            bounds->max.x = (u16)(dimensions->h_width * right);
            bounds->max.y = (u16)(dimensions->h_height * bottom);
            bounds->min.y = (u16)(dimensions->h_height * top);
            return true;
        }
    }

    return false;
}

INLINE void updatePrimitiveSSB(Scene *scene, Viewport *viewport, Primitive *primitive) {
    f32 radius, min_r, max_r;
    AABB *aabb;

    xform3 *xform = &viewport->camera->transform;
    vec3 view_space_position = subVec3(primitive->position, xform->position);
    view_space_position = mulVec3Mat3(view_space_position, xform->rotation_matrix_inverted);

    switch (primitive->type) {
        case PrimitiveType_Quad       : radius = lengthVec3(  primitive->scale);         break;
        case PrimitiveType_Box        : radius = lengthVec3(  primitive->scale);         break;
        case PrimitiveType_Tetrahedron: radius = maxCoordVec3(primitive->scale) * SQRT2; break;
        case PrimitiveType_Sphere     : radius = maxCoordVec3(primitive->scale);         break;
        case PrimitiveType_Mesh       :
            aabb   = &scene->meshes[primitive->id].aabb;
            min_r  = lengthVec3(mulVec3(primitive->scale, aabb->min));
            max_r  = lengthVec3(mulVec3(primitive->scale, aabb->max));
            radius = max_r > min_r ? max_r : min_r;
            break;
        default:
            return;
    }

    primitive->flags &= ~IS_VISIBLE;
    if (computeSSB(&primitive->screen_bounds,
                   &view_space_position, radius,
                   viewport->camera->focal_length,
                   &viewport->frame_buffer->dimensions))
        primitive->flags |= IS_VISIBLE;
}

void updateSceneSSB(Scene *scene, Viewport *viewport) {
    for (u32 i = 0; i < scene->settings.primitives; i++)
        updatePrimitiveSSB(scene, viewport, scene->primitives + i);

    uploadPrimitives(scene);
}

void drawSSB(Scene *scene, Viewport *viewport) {
    RGBA color;
    Primitive *primitive = scene->primitives;
    vec2i min, max;
    for (u32 i = 0; i < scene->settings.primitives; i++, primitive++) {
        if (primitive->flags & IS_VISIBLE) {
            switch (primitive->type) {
                case PrimitiveType_Box        : color = ColorOf(Cyan);    break;
                case PrimitiveType_Quad       : color = ColorOf(White);   break;
                case PrimitiveType_Sphere     : color = ColorOf(Yellow);  break;
                case PrimitiveType_Tetrahedron: color = ColorOf(Magenta); break;
                case PrimitiveType_Mesh       : color = ColorOf(Red);     break;
                default:
                    continue;
            }
            min = primitive->screen_bounds.min;
            max = primitive->screen_bounds.max;

            drawHLine(viewport->frame_buffer, color, min.x, max.x, min.y);
            drawHLine(viewport->frame_buffer, color, min.x, max.x, max.y);
            drawVLine(viewport->frame_buffer, color, min.y, max.y, min.x);
            drawVLine(viewport->frame_buffer, color, min.y, max.y, max.x);
        }
    }
}