#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "math/types.h"
#include "math/float2_funcs.h"
#include "math/float4_funcs.h"
#include "math/color.h"
#include "rendering/texture.h"

pim_inline float2 VEC_CALL TransformUv(float2 uv, float4 st)
{
    uv.x = uv.x * st.x + st.z;
    uv.y = uv.y * st.y + st.w;
    return uv;
}

pim_inline float4 VEC_CALL Tex_Nearesti2(texture_t texture, int2 coord)
{
    coord.x = coord.x & (texture.width - 1);
    coord.y = coord.y & (texture.height - 1);
    i32 i = coord.x + coord.y * texture.width;
    u32 color = texture.texels[i];
    return f4_tolinear(rgba8_f4(color));
}

pim_inline float4 VEC_CALL Tex_Nearestf2(texture_t texture, float2 uv)
{
    uv = f2_sub(f2_mul(uv, f2_iv(texture.width, texture.height)), f2_s(0.5f));
    return Tex_Nearesti2(texture, f2_i2(uv));
}

pim_inline float4 VEC_CALL Tex_Bilinearf2(texture_t texture, float2 uv)
{
    uv.x = uv.x * texture.width;
    uv.y = uv.y * texture.height;
    float2 frac = f2_frac(uv);
    int2 ia = f2_i2(uv);
    int2 ib = { ia.x + 1, ia.y + 0 };
    int2 ic = { ia.x + 0, ia.y + 1 };
    int2 id = { ia.x + 1, ia.y + 1 };
    float4 a = Tex_Nearesti2(texture, ia);
    float4 b = Tex_Nearesti2(texture, ib);
    float4 c = Tex_Nearesti2(texture, ic);
    float4 d = Tex_Nearesti2(texture, id);
    float4 e = f4_lerp(f4_lerp(a, b, frac.x), f4_lerp(c, d, frac.x), frac.y);
    return e;
}

PIM_C_END
