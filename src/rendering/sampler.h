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
    return f2_addvs(f2_mul(uv, i2_f2(size)), -0.5f);
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
    i32 index = UvClamp(size, uv);
    u32 color = buffer[index];
    float4 linear = ColorToLinear(color);
    return linear;
}
pim_inline float4 VEC_CALL UvWrap_c32(const u32* buffer, int2 size, float2 uv)
{
    i32 index = UvWrap(size, uv);
    u32 color = buffer[index];
    float4 linear = ColorToLinear(color);
    return linear;
}
pim_inline float4 VEC_CALL UvWrap_dir8(const u32* buffer, int2 size, float2 uv)
{
    i32 index = UvWrap(size, uv);
    u32 dir8 = buffer[index];
    float4 dir = ColorToDirection(dir8);
    return dir;
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
    a = f4_lerpvs(a, b, frac.x);
    b = f4_lerpvs(c, d, frac.x);
    c = f4_lerpvs(a, b, frac.y);
    return c;
}

pim_inline float3 VEC_CALL BilinearBlend_f3(
    float3 a, float3 b, float3 c, float3 d,
    float2 frac)
{
    a = f3_lerp(a, b, frac.x);
    b = f3_lerp(c, d, frac.x);
    c = f3_lerp(a, b, frac.y);
    return c;
}

pim_inline float2 VEC_CALL BilinearBlend_f2(
    float2 a, float2 b, float2 c, float2 d,
    float2 frac)
{
    a = f2_lerp(a, b, frac.x);
    b = f2_lerp(c, d, frac.x);
    c = f2_lerp(a, b, frac.y);
    return c;
}

pim_inline float VEC_CALL BilinearBlend_f1(
    float a, float b, float c, float d,
    float2 frac)
{
    a = f1_lerp(a, b, frac.x);
    b = f1_lerp(c, d, frac.x);
    c = f1_lerp(a, b, frac.y);
    return c;
}

pim_inline float4 VEC_CALL BilinearClamp_f4(const float4* pim_noalias buffer, int2 size, bilinear_t bi)
{
    i32 ia = Clamp(size, bi.a);
    i32 ib = Clamp(size, bi.b);
    i32 ic = Clamp(size, bi.c);
    i32 id = Clamp(size, bi.d);
    float4 a = buffer[ia];
    float4 b = buffer[ib];
    float4 c = buffer[ic];
    float4 d = buffer[id];
    return BilinearBlend_f4(a, b, c, d, bi.frac);
}
pim_inline float4 VEC_CALL BilinearWrap_f4(const float4* pim_noalias buffer, int2 size, bilinear_t bi)
{
    i32 ia = Wrap(size, bi.a);
    i32 ib = Wrap(size, bi.b);
    i32 ic = Wrap(size, bi.c);
    i32 id = Wrap(size, bi.d);
    float4 a = buffer[ia];
    float4 b = buffer[ib];
    float4 c = buffer[ic];
    float4 d = buffer[id];
    return BilinearBlend_f4(a, b, c, d, bi.frac);
}

pim_inline float3 VEC_CALL BilinearClamp_f3(const float3* pim_noalias buffer, int2 size, bilinear_t bi)
{
    i32 ia = Clamp(size, bi.a);
    i32 ib = Clamp(size, bi.b);
    i32 ic = Clamp(size, bi.c);
    i32 id = Clamp(size, bi.d);
    float3 a = buffer[ia];
    float3 b = buffer[ib];
    float3 c = buffer[ic];
    float3 d = buffer[id];
    return BilinearBlend_f3(a, b, c, d, bi.frac);
}
pim_inline float3 VEC_CALL BilinearWrap_f3(const float3* pim_noalias buffer, int2 size, bilinear_t bi)
{
    i32 ia = Wrap(size, bi.a);
    i32 ib = Wrap(size, bi.b);
    i32 ic = Wrap(size, bi.c);
    i32 id = Wrap(size, bi.d);
    float3 a = buffer[ia];
    float3 b = buffer[ib];
    float3 c = buffer[ic];
    float3 d = buffer[id];
    return BilinearBlend_f3(a, b, c, d, bi.frac);
}

pim_inline float2 VEC_CALL BilinearClamp_f2(const float2* pim_noalias buffer, int2 size, bilinear_t bi)
{
    i32 ia = Clamp(size, bi.a);
    i32 ib = Clamp(size, bi.b);
    i32 ic = Clamp(size, bi.c);
    i32 id = Clamp(size, bi.d);
    float2 a = buffer[ia];
    float2 b = buffer[ib];
    float2 c = buffer[ic];
    float2 d = buffer[id];
    return BilinearBlend_f2(a, b, c, d, bi.frac);
}
pim_inline float2 VEC_CALL BilinearWrap_f2(const float2* pim_noalias buffer, int2 size, bilinear_t bi)
{
    i32 ia = Wrap(size, bi.a);
    i32 ib = Wrap(size, bi.b);
    i32 ic = Wrap(size, bi.c);
    i32 id = Wrap(size, bi.d);
    float2 a = buffer[ia];
    float2 b = buffer[ib];
    float2 c = buffer[ic];
    float2 d = buffer[id];
    return BilinearBlend_f2(a, b, c, d, bi.frac);
}

pim_inline float VEC_CALL BilinearClamp_f1(const float* pim_noalias buffer, int2 size, bilinear_t bi)
{
    i32 ia = Clamp(size, bi.a);
    i32 ib = Clamp(size, bi.b);
    i32 ic = Clamp(size, bi.c);
    i32 id = Clamp(size, bi.d);
    float a = buffer[ia];
    float b = buffer[ib];
    float c = buffer[ic];
    float d = buffer[id];
    return BilinearBlend_f1(a, b, c, d, bi.frac);
}
pim_inline float VEC_CALL BilinearWrap_f1(const float* pim_noalias buffer, int2 size, bilinear_t bi)
{
    i32 ia = Wrap(size, bi.a);
    i32 ib = Wrap(size, bi.b);
    i32 ic = Wrap(size, bi.c);
    i32 id = Wrap(size, bi.d);
    float a = buffer[ia];
    float b = buffer[ib];
    float c = buffer[ic];
    float d = buffer[id];
    return BilinearBlend_f1(a, b, c, d, bi.frac);
}

pim_inline float4 VEC_CALL BilinearClamp_c32(const u32* pim_noalias buffer, int2 size, bilinear_t bi)
{
    float4 a = Clamp_c32(buffer, size, bi.a);
    float4 b = Clamp_c32(buffer, size, bi.b);
    float4 c = Clamp_c32(buffer, size, bi.c);
    float4 d = Clamp_c32(buffer, size, bi.d);
    return BilinearBlend_f4(a, b, c, d, bi.frac);
}
pim_inline float4 VEC_CALL BilinearWrap_c32(const u32* pim_noalias buffer, int2 size, bilinear_t bi)
{
    float4 a = Wrap_c32(buffer, size, bi.a);
    float4 b = Wrap_c32(buffer, size, bi.b);
    float4 c = Wrap_c32(buffer, size, bi.c);
    float4 d = Wrap_c32(buffer, size, bi.d);
    return BilinearBlend_f4(a, b, c, d, bi.frac);
}

pim_inline float4 VEC_CALL UvBilinearClamp_f4(const float4* pim_noalias buffer, int2 size, float2 uv)
{
    bilinear_t bi = Bilinear(size, uv);
    float4 value = BilinearClamp_f4(buffer, size, bi);
    return value;
}
pim_inline float4 VEC_CALL UvBilinearWrap_f4(const float4* pim_noalias buffer, int2 size, float2 uv)
{
    bilinear_t bi = Bilinear(size, uv);
    float4 value = BilinearWrap_f4(buffer, size, bi);
    return value;
}

pim_inline float3 VEC_CALL UvBilinearClamp_f3(const float3* pim_noalias buffer, int2 size, float2 uv)
{
    bilinear_t bi = Bilinear(size, uv);
    float3 value = BilinearClamp_f3(buffer, size, bi);
    return value;
}
pim_inline float3 VEC_CALL UvBilinearWrap_f3(const float3* pim_noalias buffer, int2 size, float2 uv)
{
    bilinear_t bi = Bilinear(size, uv);
    float3 value = BilinearWrap_f3(buffer, size, bi);
    return value;
}

pim_inline float2 VEC_CALL UvBilinearClamp_f2(const float2* pim_noalias buffer, int2 size, float2 uv)
{
    bilinear_t bi = Bilinear(size, uv);
    float2 value = BilinearClamp_f2(buffer, size, bi);
    return value;
}
pim_inline float2 VEC_CALL UvBilinearWrap_f2(const float2* pim_noalias buffer, int2 size, float2 uv)
{
    bilinear_t bi = Bilinear(size, uv);
    float2 value = BilinearWrap_f2(buffer, size, bi);
    return value;
}

pim_inline float VEC_CALL UvBilinearClamp_f1(const float* pim_noalias buffer, int2 size, float2 uv)
{
    bilinear_t bi = Bilinear(size, uv);
    float value = BilinearClamp_f1(buffer, size, bi);
    return value;
}
pim_inline float VEC_CALL UvBilinearWrap_f1(const float* pim_noalias buffer, int2 size, float2 uv)
{
    bilinear_t bi = Bilinear(size, uv);
    float value = BilinearWrap_f1(buffer, size, bi);
    return value;
}

pim_inline float4 VEC_CALL UvBilinearClamp_c32(const u32* pim_noalias buffer, int2 size, float2 uv)
{
    bilinear_t bi = Bilinear(size, uv);
    float4 value = BilinearClamp_c32(buffer, size, bi);
    return value;
}
pim_inline float4 VEC_CALL UvBilinearWrap_c32(const u32* pim_noalias buffer, int2 size, float2 uv)
{
    bilinear_t bi = Bilinear(size, uv);
    float4 value = BilinearWrap_c32(buffer, size, bi);
    return value;
}

pim_inline float4 VEC_CALL UvBilinearWrap_dir8(const u32* pim_noalias buffer, int2 size, float2 uv)
{
    bilinear_t bi = Bilinear(size, uv);
    i32 ia = Wrap(size, bi.a);
    i32 ib = Wrap(size, bi.b);
    i32 ic = Wrap(size, bi.c);
    i32 id = Wrap(size, bi.d);
    u32 ca = buffer[ia];
    u32 cb = buffer[ib];
    u32 cc = buffer[ic];
    u32 cd = buffer[id];
    float4 a = ColorToDirection(ca);
    float4 b = ColorToDirection(cb);
    float4 c = ColorToDirection(cc);
    float4 d = ColorToDirection(cd);
    float4 N = BilinearBlend_f4(a, b, c, d, bi.frac);
    return f4_normalize3(N);
}

pim_inline float4 VEC_CALL TrilinearClamp_f4(
    const float4* pim_noalias buffer,
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
    const float4* pim_noalias buffer,
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
    const u32* pim_noalias buffer,
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
    const u32* pim_noalias buffer,
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

pim_inline void VEC_CALL Write_f4(float4* pim_noalias dst, int2 size, int2 coord, float4 src)
{
    i32 i = Clamp(size, coord);
    dst[i] = src;
}

pim_inline void VEC_CALL Write_c32(u32* pim_noalias dst, int2 size, int2 coord, float4 src)
{
    i32 i = Clamp(size, coord);
    dst[i] = LinearToColor(src);
}

PIM_C_END
