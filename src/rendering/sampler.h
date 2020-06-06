#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "math/types.h"
#include "math/int2_funcs.h"
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

pim_inline i32 VEC_CALL CalcMipOffset(int2 size, i32 m)
{
    i32 y = 0;
    m = i1_clamp(m, 0, CalcMipCount(size) - 1);
    for (i32 i = 0; i < m; ++i)
    {
        int2 s = CalcMipSize(size, i);
        y += s.x * s.y;
    }
    return y;
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
    return ColorToLinear(buffer[Clamp(size, coord)]);
}
pim_inline float4 VEC_CALL Wrap_c32(const u32* buffer, int2 size, int2 coord)
{
    return ColorToLinear(buffer[Wrap(size, coord)]);
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

pim_inline float2 VEC_CALL BilinearBlend_f2(
    float2 a, float2 b, float2 c, float2 d,
    float2 frac)
{
    return f2_lerp(f2_lerp(a, b, frac.x), f2_lerp(c, d, frac.x), frac.y);
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

pim_inline float2 VEC_CALL BilinearClamp_f2(const float2* buffer, int2 size, bilinear_t bi)
{
    float2 a = buffer[Clamp(size, bi.a)];
    float2 b = buffer[Clamp(size, bi.b)];
    float2 c = buffer[Clamp(size, bi.c)];
    float2 d = buffer[Clamp(size, bi.d)];
    return BilinearBlend_f2(a, b, c, d, bi.frac);
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

pim_inline float2 VEC_CALL UvBilinearClamp_f2(const float2* buffer, int2 size, float2 uv)
{
    return BilinearClamp_f2(buffer, size, Bilinear(size, uv));
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
