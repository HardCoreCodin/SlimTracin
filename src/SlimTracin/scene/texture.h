#pragma once

#include "../core/types.h"

INLINE vec4 sampleTextureMip(TextureMip *mip, vec2 UV) {
    f32 u = UV.u;
    f32 v = UV.v;
    if (u > 1) u -= (f32)((u32)u);
    if (v > 1) v -= (f32)((u32)v);

    const f32 U = u * (f32)mip->width  + 0.5f;
    const f32 V = v * (f32)mip->height + 0.5f;
    const u32 x = (u32)U;
    const u32 y = (u32)V;
    const f32 r = U - (f32)x;
    const f32 b = V - (f32)y;
    const f32 l = 1 - r;
    const f32 t = 1 - b;
    const f32 tl = t * l * COLOR_COMPONENT_TO_FLOAT;
    const f32 tr = t * r * COLOR_COMPONENT_TO_FLOAT;
    const f32 bl = b * l * COLOR_COMPONENT_TO_FLOAT;
    const f32 br = b * r * COLOR_COMPONENT_TO_FLOAT;

    const TexelQuad texel_quad = mip->texel_quads[y * (mip->width + 1) + x];
    return Vec4(
            fast_mul_add((f32)texel_quad.R.BR, br, fast_mul_add((f32)texel_quad.R.BL, bl, fast_mul_add((f32)texel_quad.R.TR, tr, (f32)texel_quad.R.TL * tl))),
            fast_mul_add((f32)texel_quad.G.BR, br, fast_mul_add((f32)texel_quad.G.BL, bl, fast_mul_add((f32)texel_quad.G.TR, tr, (f32)texel_quad.G.TL * tl))),
            fast_mul_add((f32)texel_quad.B.BR, br, fast_mul_add((f32)texel_quad.B.BL, bl, fast_mul_add((f32)texel_quad.B.TR, tr, (f32)texel_quad.B.TL * tl))),
            1.0f);
}

INLINE vec4 sampleTexture(Texture *texture, vec2 UV, vec2 dUV) {
    u8 mip_level = 0;
    if (texture->mipmap) {
        const f32 pixel_width  = dUV.u * (f32)texture->width;
        const f32 pixel_height = dUV.v * (f32)texture->height;
        f32 pixel_size = (f32)(pixel_width + pixel_height) * 0.5f;
        const u8 last_mip = texture->mip_count - 1;

        while (pixel_size > 1 && mip_level < last_mip) {
            pixel_size /= 2;
            mip_level += 1;
        }
    }

    return sampleTextureMip(texture->mips + mip_level, UV);
}