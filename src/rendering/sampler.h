#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "math/types.h"
#include "math/int2_funcs.h"
#include "math/float2_funcs.h"
#include "math/float3_funcs.h"
#include "math/float4_funcs.h"
#include "math/color.h"
#include "rendering/texture.h"

pim_inline float2 VEC_CALL TransformUv(float2 uv, float4 st)
{
    uv.x = uv.x * st.x + st.z;
    uv.y = uv.y * st.y + st.w;
    return uv;
}

pim_inline i32 VEC_CALL CalcMipCount(int2 size)
{
    return 1 + i1_log2(i1_max(size.x, size.y));
}

pim_inline int2 VEC_CALL CalcMipSize(int2 size, i32 m)
{
    int2 y;
    y.x = i1_max(1, size.x >> m);
    y.y = i1_max(1, size.y >> m);
    return y;
}

pim_inline i32 VEC_CALL CalcMipLen(int2 size, i32 m)
{
    int2 mipSize = CalcMipSize(size, m);
    return mipSize.x * mipSize.y;
}

pim_inline i32 VEC_CALL CalcMipOffset(int2 size, i32 m)
{
    i32 y = 0;
    m = i1_clamp(m, 0, CalcMipCount(size) - 1);
    for (i32 i = 0; i < m; ++i)
    {
        y += CalcMipLen(size, i);
    }
    return y;
}

pim_inline i32 VEC_CALL MipChainLen(int2 size)
{
    i32 len = 0;
    i32 mipCount = CalcMipCount(size);
    for (i32 i = 0; i < mipCount; ++i)
    {
        len += CalcMipLen(size, i);
    }
    return len;
}

// stride: number of texels traveled per sample in x and y
pim_inline float VEC_CALL CalcMipLevel(float2 stride)
{
    return f1_max(0.0f, 0.5f * log2f(f2_dot(stride, stride)));
}

pim_inline i32 VEC_CALL CalcTileMip(int2 tileSize)
{
    float m = CalcMipLevel(i2_f2(tileSize));
    return (i32)(m + 0.5f);
}

pim_inline float2 VEC_CALL UvToCoordf(int2 size, float2 uv)
{
    return f2_addvs(f2_mul(uv, i2_f2(size)), 0.5f);
}
pim_inline int2 VEC_CALL UvToCoord(int2 size, float2 uv)
{
    return f2_i2(UvToCoordf(size, uv));
}

pim_inline float2 VEC_CALL CoordToUv(int2 size, int2 coord)
{
    float2 uv = f2_addvs(i2_f2(coord), 0.5f);
    return f2_div(uv, i2_f2(size));
}

pim_inline int2 VEC_CALL ClampCoord(int2 size, int2 coord)
{
    return i2_clamp(coord, i2_0, i2_subvs(size, 1));
}
pim_inline int2 VEC_CALL WrapCoord(int2 size, int2 coord)
{
    return i2_mod(coord, size);
}

pim_inline i32 VEC_CALL CoordToIndex(int2 size, int2 coord)
{
    return coord.x + coord.y * size.x;
}

pim_inline int2 VEC_CALL IndexToCoord(int2 size, i32 index)
{
    return i2_v(index % size.x, index / size.x);
}

pim_inline float2 VEC_CALL IndexToUv(int2 size, i32 index)
{
    return CoordToUv(size, IndexToCoord(size, index));
}

pim_inline i32 VEC_CALL UvToIndex(int2 size, float2 uv)
{
    return CoordToIndex(size, UvToCoord(size, uv));
}

pim_inline i32 VEC_CALL Clamp(int2 size, int2 coord)
{
    return CoordToIndex(size, ClampCoord(size, coord));
}
pim_inline i32 VEC_CALL Wrap(int2 size, int2 coord)
{
    return CoordToIndex(size, WrapCoord(size, coord));
}
pim_inline i32 VEC_CALL UvClamp(int2 size, float2 uv)
{
    return Clamp(size, UvToCoord(size, uv));
}
pim_inline i32 VEC_CALL UvWrap(int2 size, float2 uv)
{
    return Wrap(size, UvToCoord(size, uv));
}

pim_inline float4 VEC_CALL Clamp_c32(const u32* buffer, int2 size, int2 coord)
{
    i32 index = Clamp(size, coord);
    u32 color = buffer[index];
    float4 linear = ColorToLinear(color);
    return linear;
}
pim_inline float4 VEC_CALL Wrap_c32(const u32* buffer, int2 size, int2 coord)
{
    i32 index = Wrap(size, coord);
    u32 color = buffer[index];
    float4 linear = ColorToLinear(color);
    return linear;
}
pim_inline float4 VEC_CALL UvClamp_c32(const u32* buffer, int2 size, float2 uv)
{
    return ColorToLinear(buffer[UvClamp(size, uv)]);
}
pim_inline float4 VEC_CALL UvWrap_c32(const u32* buffer, int2 size, float2 uv)
{
    return ColorToLinear(buffer[UvWrap(size, uv)]);
}

typedef struct bilinear_s
{
    int2 a;
    int2 b;
    int2 c;
    int2 d;
    float2 frac;
} bilinear_t;

pim_inline bilinear_t VEC_CALL Bilinear(int2 size, float2 uv)
{
    bilinear_t b;
    float2 coordf = UvToCoordf(size, uv);
    b.frac = f2_frac(coordf);
    b.a = f2_i2(coordf);
    b.b = i2_v(b.a.x + 1, b.a.y + 0);
    b.c = i2_v(b.a.x + 0, b.a.y + 1);
    b.d = i2_v(b.a.x + 1, b.a.y + 1);
    return b;
}

pim_inline float4 VEC_CALL BilinearBlend_f4(
    float4 a, float4 b, float4 c, float4 d,
    float2 frac)
{
    return f4_lerpvs(f4_lerpvs(a, b, frac.x), f4_lerpvs(c, d, frac.x), frac.y);
}

pim_inline float3 VEC_CALL BilinearBlend_f3(
    float3 a, float3 b, float3 c, float3 d,
    float2 frac)
{
    return f3_lerp(f3_lerp(a, b, frac.x), f3_lerp(c, d, frac.x), frac.y);
}

pim_inline float2 VEC_CALL BilinearBlend_f2(
    float2 a, float2 b, float2 c, float2 d,
    float2 frac)
{
    return f2_lerp(f2_lerp(a, b, frac.x), f2_lerp(c, d, frac.x), frac.y);
}

pim_inline float VEC_CALL BilinearBlend_f1(
    float a, float b, float c, float d,
    float2 frac)
{
    return f1_lerp(f1_lerp(a, b, frac.x), f1_lerp(c, d, frac.x), frac.y);
}

pim_inline float4 VEC_CALL BilinearClamp_f4(const float4* buffer, int2 size, bilinear_t bi)
{
    float4 a = buffer[Clamp(size, bi.a)];
    float4 b = buffer[Clamp(size, bi.b)];
    float4 c = buffer[Clamp(size, bi.c)];
    float4 d = buffer[Clamp(size, bi.d)];
    return BilinearBlend_f4(a, b, c, d, bi.frac);
}
pim_inline float4 VEC_CALL BilinearWrap_f4(const float4* buffer, int2 size, bilinear_t bi)
{
    float4 a = buffer[Wrap(size, bi.a)];
    float4 b = buffer[Wrap(size, bi.b)];
    float4 c = buffer[Wrap(size, bi.c)];
    float4 d = buffer[Wrap(size, bi.d)];
    return BilinearBlend_f4(a, b, c, d, bi.frac);
}

pim_inline float3 VEC_CALL BilinearClamp_f3(const float3* buffer, int2 size, bilinear_t bi)
{
    float3 a = buffer[Clamp(size, bi.a)];
    float3 b = buffer[Clamp(size, bi.b)];
    float3 c = buffer[Clamp(size, bi.c)];
    float3 d = buffer[Clamp(size, bi.d)];
    return BilinearBlend_f3(a, b, c, d, bi.frac);
}
pim_inline float3 VEC_CALL BilinearWrap_f3(const float3* buffer, int2 size, bilinear_t bi)
{
    float3 a = buffer[Wrap(size, bi.a)];
    float3 b = buffer[Wrap(size, bi.b)];
    float3 c = buffer[Wrap(size, bi.c)];
    float3 d = buffer[Wrap(size, bi.d)];
    return BilinearBlend_f3(a, b, c, d, bi.frac);
}

pim_inline float2 VEC_CALL BilinearClamp_f2(const float2* buffer, int2 size, bilinear_t bi)
{
    float2 a = buffer[Clamp(size, bi.a)];
    float2 b = buffer[Clamp(size, bi.b)];
    float2 c = buffer[Clamp(size, bi.c)];
    float2 d = buffer[Clamp(size, bi.d)];
    return BilinearBlend_f2(a, b, c, d, bi.frac);
}

pim_inline float VEC_CALL BilinearClamp_f1(const float* buffer, int2 size, bilinear_t bi)
{
    float a = buffer[Clamp(size, bi.a)];
    float b = buffer[Clamp(size, bi.b)];
    float c = buffer[Clamp(size, bi.c)];
    float d = buffer[Clamp(size, bi.d)];
    return BilinearBlend_f1(a, b, c, d, bi.frac);
}


pim_inline float4 VEC_CALL BilinearClamp_c32(const u32* buffer, int2 size, bilinear_t bi)
{
    float4 a = Clamp_c32(buffer, size, bi.a);
    float4 b = Clamp_c32(buffer, size, bi.b);
    float4 c = Clamp_c32(buffer, size, bi.c);
    float4 d = Clamp_c32(buffer, size, bi.d);
    return BilinearBlend_f4(a, b, c, d, bi.frac);
}
pim_inline float4 VEC_CALL BilinearWrap_c32(const u32* buffer, int2 size, bilinear_t bi)
{
    float4 a = Wrap_c32(buffer, size, bi.a);
    float4 b = Wrap_c32(buffer, size, bi.b);
    float4 c = Wrap_c32(buffer, size, bi.c);
    float4 d = Wrap_c32(buffer, size, bi.d);
    return BilinearBlend_f4(a, b, c, d, bi.frac);
}

pim_inline float4 VEC_CALL UvBilinearClamp_f4(const float4* buffer, int2 size, float2 uv)
{
    return BilinearClamp_f4(buffer, size, Bilinear(size, uv));
}
pim_inline float4 VEC_CALL UvBilinearWrap_f4(const float4* buffer, int2 size, float2 uv)
{
    return BilinearWrap_f4(buffer, size, Bilinear(size, uv));
}

pim_inline float3 VEC_CALL UvBilinearClamp_f3(const float3* buffer, int2 size, float2 uv)
{
    return BilinearClamp_f3(buffer, size, Bilinear(size, uv));
}
pim_inline float3 VEC_CALL UvBilinearWrap_f3(const float3* buffer, int2 size, float2 uv)
{
    return BilinearWrap_f3(buffer, size, Bilinear(size, uv));
}

pim_inline float2 VEC_CALL UvBilinearClamp_f2(const float2* buffer, int2 size, float2 uv)
{
    return BilinearClamp_f2(buffer, size, Bilinear(size, uv));
}

pim_inline float VEC_CALL UvBilinearClamp_f1(const float* buffer, int2 size, float2 uv)
{
    return BilinearClamp_f1(buffer, size, Bilinear(size, uv));
}

pim_inline float4 VEC_CALL UvBilinearClamp_c32(const u32* buffer, int2 size, float2 uv)
{
    return BilinearClamp_c32(buffer, size, Bilinear(size, uv));
}
pim_inline float4 VEC_CALL UvBilinearWrap_c32(const u32* buffer, int2 size, float2 uv)
{
    return BilinearWrap_c32(buffer, size, Bilinear(size, uv));
}

pim_inline float4 VEC_CALL UvBilinearWrap_dir8(const u32* buffer, int2 size, float2 uv)
{
    bilinear_t bi = Bilinear(size, uv);
    float4 a = rgba8_dir(buffer[Wrap(size, bi.a)]);
    float4 b = rgba8_dir(buffer[Wrap(size, bi.b)]);
    float4 c = rgba8_dir(buffer[Wrap(size, bi.c)]);
    float4 d = rgba8_dir(buffer[Wrap(size, bi.d)]);
    float4 N = BilinearBlend_f4(a, b, c, d, bi.frac);
    return f4_normalize3(N);
}

pim_inline float4 VEC_CALL TrilinearClamp_f4(
    const float4* buffer,
    int2 size,
    float2 uv,
    float mip)
{
    i32 m0 = (i32)f1_floor(mip);
    i32 m1 = (i32)f1_ceil(mip);
    float mfrac = f1_frac(mip);

    int2 size0 = CalcMipSize(size, m0);
    int2 size1 = CalcMipSize(size, m1);

    i32 i0 = CalcMipOffset(size, m0);
    i32 i1 = CalcMipOffset(size, m1);

    float4 s0 = UvBilinearClamp_f4(buffer + i0, size0, uv);
    float4 s1 = UvBilinearClamp_f4(buffer + i1, size1, uv);

    return f4_lerpvs(s0, s1, mfrac);
}
pim_inline float4 VEC_CALL TrilinearWrap_f4(
    const float4* buffer,
    int2 size,
    float2 uv,
    float mip)
{
    i32 m0 = (i32)f1_floor(mip);
    i32 m1 = (i32)f1_ceil(mip);
    float mfrac = f1_frac(mip);

    int2 size0 = CalcMipSize(size, m0);
    int2 size1 = CalcMipSize(size, m1);

    i32 i0 = CalcMipOffset(size, m0);
    i32 i1 = CalcMipOffset(size, m1);

    float4 s0 = UvBilinearWrap_f4(buffer + i0, size0, uv);
    float4 s1 = UvBilinearWrap_f4(buffer + i1, size1, uv);

    return f4_lerpvs(s0, s1, mfrac);
}

pim_inline float4 VEC_CALL TrilinearClamp_c32(
    const u32* buffer,
    int2 size,
    float2 uv,
    float mip)
{
    i32 m0 = (i32)f1_floor(mip);
    i32 m1 = (i32)f1_ceil(mip);
    float mfrac = f1_frac(mip);

    int2 size0 = CalcMipSize(size, m0);
    int2 size1 = CalcMipSize(size, m1);

    i32 i0 = CalcMipOffset(size, m0);
    i32 i1 = CalcMipOffset(size, m1);

    float4 s0 = UvBilinearClamp_c32(buffer + i0, size0, uv);
    float4 s1 = UvBilinearClamp_c32(buffer + i1, size1, uv);

    return f4_lerpvs(s0, s1, mfrac);
}
pim_inline float4 VEC_CALL TrilinearWrap_c32(
    const u32* buffer,
    int2 size,
    float2 uv,
    float mip)
{
    i32 m0 = (i32)f1_floor(mip);
    i32 m1 = (i32)f1_ceil(mip);
    float mfrac = f1_frac(mip);

    int2 size0 = CalcMipSize(size, m0);
    int2 size1 = CalcMipSize(size, m1);

    i32 i0 = CalcMipOffset(size, m0);
    i32 i1 = CalcMipOffset(size, m1);

    float4 s0 = UvBilinearWrap_c32(buffer + i0, size0, uv);
    float4 s1 = UvBilinearWrap_c32(buffer + i1, size1, uv);

    return f4_lerpvs(s0, s1, mfrac);
}

pim_inline float4 VEC_CALL CubicEq(float t)
{
    float4 n = f4_subvs(f4_v(1.0f, 2.0f, 3.0f, 4.0f), t);
    float4 s = f4_mul(f4_mul(n, n), n);
    float x = s.x;
    float y = s.y - 4.0f * s.x;
    float z = s.z - 4.0f * s.y + 6.0f * s.x;
    float w = 6.0f - x - y - z;
    return f4_mulvs(f4_v(x, y, z, w), 1.0f / 6.0f);
}

// https://gist.github.com/TheRealMJP/c83b8c0f46b63f3a88a5986f4fa982b1
pim_inline float3 VEC_CALL UvBicubic_f3(
    const float3* pim_noalias texels,
    int2 size,
    float2 uv)
{
    const float2 sizef = { (float)size.x, (float)size.y };
    const float2 rcpSize = f2_rcp(sizef);

    float2 samplePos = f2_mul(uv, sizef);
    float2 texPos1 = f2_addvs(f2_floor(f2_subvs(samplePos, 0.5f)), 0.5f);
    float2 f = f2_sub(samplePos, texPos1);

    float2 w0 =
    {
        f.x * (-0.5f + f.x * (1.0f - 0.5f * f.x)),
        f.y * (-0.5f + f.y * (1.0f - 0.5f * f.y)),
    };
    float2 w1 =
    {
        1.0f + f.x * f.x * (-2.5f + 1.5f * f.x),
        1.0f + f.y * f.y * (-2.5f + 1.5f * f.y),
    };
    float2 w2 =
    {
        f.x * (0.5f + f.x * (2.0f - 1.5f * f.x)),
        f.y * (0.5f + f.y * (2.0f - 1.5f * f.y)),
    };
    float2 w3 =
    {
        f.x * f.x * (-0.5f + 0.5f * f.x),
        f.y * f.y * (-0.5f + 0.5f * f.y),
    };

    float2 w12 = f2_add(w1, w2);
    float2 offset12 = f2_div(w2, w12);

    float2 texPos0 = f2_subvs(texPos1, 1.0f);
    float2 texPos3 = f2_addvs(texPos1, 2.0f);
    float2 texPos12 = f2_add(texPos1, offset12);

    texPos0 = f2_mul(texPos0, rcpSize);
    texPos3 = f2_mul(texPos3, rcpSize);
    texPos12 = f2_mul(texPos12, rcpSize);

    float3 result = f3_0;
    float3 sample;

    sample = UvBilinearClamp_f3(texels, size, f2_v(texPos0.x, texPos0.y));
    result = f3_add(result, f3_mulvs(sample, w0.x * w0.y));
    sample = UvBilinearClamp_f3(texels, size, f2_v(texPos12.x, texPos0.y));
    result = f3_add(result, f3_mulvs(sample, w12.x * w0.y));
    sample = UvBilinearClamp_f3(texels, size, f2_v(texPos3.x, texPos0.y));
    result = f3_add(result, f3_mulvs(sample, w3.x * w0.y));

    sample = UvBilinearClamp_f3(texels, size, f2_v(texPos0.x, texPos12.y));
    result = f3_add(result, f3_mulvs(sample, w0.x * w12.y));
    sample = UvBilinearClamp_f3(texels, size, f2_v(texPos12.x, texPos12.y));
    result = f3_add(result, f3_mulvs(sample, w12.x * w12.y));
    sample = UvBilinearClamp_f3(texels, size, f2_v(texPos3.x, texPos12.y));
    result = f3_add(result, f3_mulvs(sample, w3.x * w12.y));

    sample = UvBilinearClamp_f3(texels, size, f2_v(texPos0.x, texPos3.y));
    result = f3_add(result, f3_mulvs(sample, w0.x * w3.y));
    sample = UvBilinearClamp_f3(texels, size, f2_v(texPos12.x, texPos3.y));
    result = f3_add(result, f3_mulvs(sample, w12.x * w3.y));
    sample = UvBilinearClamp_f3(texels, size, f2_v(texPos3.x, texPos3.y));
    result = f3_add(result, f3_mulvs(sample, w3.x * w3.y));

    return result;
}

pim_inline void VEC_CALL Write_f4(float4* dst, int2 size, int2 coord, float4 src)
{
    i32 i = Clamp(size, coord);
    dst[i] = src;
}

pim_inline void VEC_CALL Write_c32(u32* dst, int2 size, int2 coord, float4 src)
{
    i32 i = Clamp(size, coord);
    dst[i] = LinearToColor(src);
}

PIM_C_END
