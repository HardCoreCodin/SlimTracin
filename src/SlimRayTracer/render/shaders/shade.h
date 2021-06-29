#pragma once

#include "./common.h"
#include "./closest_hit/debug.h"
//#include "closest_hit/classic.h"
#include "./closest_hit/surface.h"
//#include "closest_hit/reflection.h"
#include "../SSB.h"
#include "./trace.h"

INLINE void renderBeauty(Ray *ray, Trace *trace, Scene *scene, u16 x, u16 y, Pixel* pixel) {
    vec3 color = getVec3Of(0);
//    shadeReflection(scene, bvh_nodes, &tracer->masks, ray->hit.material_id, ray->direction,  &ray->hit.position, &ray->hit.normal, 0, &color);
//    if (traceSecondaryRay(ray, trace, scene, ray_tracer_args.simple_bvh_nodes))
    if (tracePrimaryRay(ray, trace, scene, x, y))
#ifdef USE_DEMO_SCENE
        color = shadeSurface(trace, scene, ray->direction, color);
#else
        color = shadeSurface2(ray, trace, scene, 1);
#endif
//    shadeLambert(scene, bvh_nodes, &tracer->masks, ray->direction, &ray->hit.position, &ray->hit.normal, &color);
//    shadePhong(scene, bvh_nodes, &tracer->masks, ray->direction, &ray->hit.position, &ray->hit.normal, &color);
//    shadeBlinn(scene, bvh_nodes, &tracer->masks, ray->direction, &ray->hit.position, &ray->hit.normal, &color);

#ifdef USE_DEMO_SCENE
    setPixelBakedToneMappedColor(pixel, color);
#else
    setPixelColor(pixel, color);
#endif
}

INLINE void renderNormals(Ray *ray, Trace *trace, Scene *scene, u16 x, u16 y, Pixel* pixel) {
    vec3 color = getVec3Of(0);
//    if (traceSecondaryRay(ray, trace, scene, ray_tracer_args.simple_bvh_nodes))
    if (tracePrimaryRay(ray, trace, scene, x, y))
        color = shadeDirection(trace->closest_hit.normal);

    setPixelColor(pixel, color);
}

INLINE void renderDepth(Ray *ray, Trace *trace, Scene *scene, u16 x, u16 y, Pixel* pixel) {
    vec3 color = getVec3Of(0);
//    if (traceSecondaryRay(ray, trace, scene, ray_tracer_args.simple_bvh_nodes))
    if (tracePrimaryRay(ray, trace, scene, x, y))
        color = shadeDepth(trace->closest_hit.distance);

    setPixelColor(pixel, color);
}

INLINE void renderUVs(Ray *ray, Trace *trace, Scene *scene, u16 x, u16 y, Pixel* pixel) {
    vec3 color = getVec3Of(0);
//    if (traceSecondaryRay(ray, trace, scene, ray_tracer_args.simple_bvh_nodes))
    if (tracePrimaryRay(ray, trace, scene, x, y))
        color = shadeUV(trace->closest_hit.uv);

    setPixelColor(pixel, color);
}