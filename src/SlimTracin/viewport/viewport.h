#pragma once

#include "../core/types.h"
#include "../math/vec3.h"
#include "./hud.h"

typedef union RGBA2u32 {
    RGBA rgba;
    u32 value;
} RGBA2u32;

void drawViewportToWindowContent(Viewport *viewport) {
    PixelGrid *frame_buffer = viewport->frame_buffer;
    u16 width = frame_buffer->dimensions.width;
    u16 height = frame_buffer->dimensions.height;
    Pixel *trg_value = frame_buffer->pixels;
    vec3 color;
    RGBA2u32 background, trg_pixel;
    background.rgba.R = (u8)(0);
    background.rgba.G = (u8)(0);
    background.rgba.B = (u8)(0);
    background.rgba.A = (u8)(FLOAT_TO_COLOR_COMPONENT);
    if (frame_buffer->QCAA) {
        u16 stride = width;
        width--;
        height--;
        FloatPixel *TL_pixel_start = frame_buffer->float_pixels;
        FloatPixel *TL_pixel, *TR_pixel, *BL_pixel, *BR_pixel;
        for (u16 y = 0; y < height; y++, TL_pixel_start += stride) {
            TL_pixel = TL_pixel_start;
            BL_pixel = TL_pixel + stride;
            TR_pixel = TL_pixel + 1;
            BR_pixel = BL_pixel + 1;

            for (u16 x = 0; x < width; x++, trg_value++, BL_pixel++, BR_pixel++, TL_pixel++, TR_pixel++) {
                if (BL_pixel->depth == INFINITY &&
                    BR_pixel->depth == INFINITY &&
                    TL_pixel->depth == INFINITY &&
                    TR_pixel->depth == INFINITY)
                    trg_pixel = background;
                else {
                    color = scaleVec3(scaleAddVec3(TL_pixel->color, TL_pixel->opacity, scaleAddVec3(TR_pixel->color, TR_pixel->opacity, scaleAddVec3(BL_pixel->color, BL_pixel->opacity, scaleVec3(BR_pixel->color, BR_pixel->opacity)))), 0.25f);
                    trg_pixel.rgba.R = (u8)(color.r > (MAX_COLOR_VALUE * MAX_COLOR_VALUE) ? MAX_COLOR_VALUE : sqrt(color.r));
                    trg_pixel.rgba.G = (u8)(color.g > (MAX_COLOR_VALUE * MAX_COLOR_VALUE) ? MAX_COLOR_VALUE : sqrt(color.g));
                    trg_pixel.rgba.B = (u8)(color.b > (MAX_COLOR_VALUE * MAX_COLOR_VALUE) ? MAX_COLOR_VALUE : sqrt(color.b));
                }
                trg_value->value = trg_pixel.value;
            }
        }
    } else {
        FloatPixel *src_pixel = frame_buffer->float_pixels;
        for (u16 y = 0; y < height; y++) {
            for (u16 x = 0; x < width; x++, src_pixel++, trg_value++) {
                if (src_pixel->depth == INFINITY)
                    trg_pixel = background;
                else {
                    color = scaleVec3(src_pixel->color, src_pixel->opacity);
                    trg_pixel.rgba.R = (u8)(color.r > (MAX_COLOR_VALUE * MAX_COLOR_VALUE) ? MAX_COLOR_VALUE : sqrt(color.r));
                    trg_pixel.rgba.G = (u8)(color.g > (MAX_COLOR_VALUE * MAX_COLOR_VALUE) ? MAX_COLOR_VALUE : sqrt(color.g));
                    trg_pixel.rgba.B = (u8)(color.b > (MAX_COLOR_VALUE * MAX_COLOR_VALUE) ? MAX_COLOR_VALUE : sqrt(color.b));
                }
                trg_value->value = trg_pixel.value;
            }
        }
    }
}

void fillViewport(Viewport *viewport, vec3 color, f32 opacity, f64 depth) {
    PixelGrid *frame_buffer = viewport->frame_buffer;
    FloatPixel *pixel = frame_buffer->float_pixels;
    i32 width = frame_buffer->dimensions.width;
    i32 height = frame_buffer->dimensions.height;
    i32 X = viewport->position.x;
    i32 Y = viewport->position.y;
    i32 X_end = X + width;
    i32 Y_end = Y + height;

    FloatPixel fill_pixel;
    fill_pixel.color = color;
    fill_pixel.opacity = opacity;
    fill_pixel.depth = depth;

    for (i32 y = Y; y < Y_end; y++)
        for (i32 x = X; x < X_end; x++, pixel++)
            *pixel = fill_pixel;
}

void clearViewportToBackground(Viewport *viewport) {
    fillViewport(viewport,
                 Color(Black),
                 0,
                 INFINITY);
}

void setProjectionMatrix(Viewport *viewport) {
    const f32 n = viewport->settings.near_clipping_plane_distance;
    const f32 f = viewport->settings.far_clipping_plane_distance;
    const f32 fl = viewport->camera->focal_length;
    const f32 ar = viewport->frame_buffer->dimensions.height_over_width;
    const f32 gl = viewport->settings.use_cube_NDC;
    mat4 M;

    M.X.y = M.X.z = M.X.w = M.Y.x = M.Y.z = M.Y.w = M.W.x = M.W.y = M.W.w = M.Z.x = M.Z.y = 0;
    M.Z.w = 1.0f;
    M.X.x = fl * ar;
    M.Y.y = fl;
    M.Z.z = M.W.z = 1.0f / (f - n);
    M.Z.z *= gl ? (f + n) : f;
    M.W.z *= gl ? (-2 * f * n) : (-n * f);

    viewport->projection_matrix = M;
}

void beginDrawing(Viewport *viewport) {
    if (viewport->settings.background_fill)
        clearViewportToBackground(viewport);

    setProjectionMatrix(viewport);
    setViewportProjectionPlane(viewport);
}

void endDrawing(Viewport *viewport) {
    if (viewport->settings.show_hud)
        drawHUD(viewport->frame_buffer, &viewport->hud);
    drawViewportToWindowContent(viewport);
}