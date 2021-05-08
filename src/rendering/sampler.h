#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "math/types.h"
#include "math/int2_funcs.h"
#include "math/float2_funcs.h"
#include "math/float3_funcs.h"
#include "math/float4_funcs.h"
#include "math/color.h"

pim_inline float2 VEC_CALL TransformUv(float2 uv, float4 st)
{
    uv.x = uv.x * st.x + st.z;
    uv.y = uv.y * st.y + st.w;
    return uv;
}

// ----------------------------------------------------------------------------

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

// ----------------------------------------------------------------------------

pim_inline float2 VEC_CALL UvToCoordf(int2 size, float2 uv)
{
    return f2_mul(uv, i2_f2(size));
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

pim_inline int2 VEC_CALL ClampCoord(int2 size, int2 coord)
{
    return i2_clamp(coord, i2_0, i2_addvs(size, -1));
}
pim_inline int2 VEC_CALL WrapCoord(int2 size, int2 coord)
{
    return i2_mod(coord, size);
}
pim_inline int2 VEC_CALL WrapCoordPow2(int2 size, int2 coord)
{
    int2 result;
    result.x = coord.x & (size.x - 1);
    result.y = coord.y & (size.y - 1);
    return result;
}

// ----------------------------------------------------------------------------

pim_inline i32 VEC_CALL Clamp(int2 size, int2 coord)
{
    return CoordToIndex(size, ClampCoord(size, coord));
}
pim_inline i32 VEC_CALL Wrap(int2 size, int2 coord)
{
    return CoordToIndex(size, WrapCoord(size, coord));
}
pim_inline i32 VEC_CALL WrapPow2(int2 size, int2 coord)
{
    return CoordToIndex(size, WrapCoordPow2(size, coord));
}

pim_inline i32 VEC_CALL UvClamp(int2 size, float2 uv)
{
    return Clamp(size, UvToCoord(size, uv));
}
pim_inline i32 VEC_CALL UvWrap(int2 size, float2 uv)
{
    return Wrap(size, UvToCoord(size, uv));
}
pim_inline i32 VEC_CALL UvWrapPow2(int2 size, float2 uv)
{
    return WrapPow2(size, UvToCoord(size, uv));
}

// ----------------------------------------------------------------------------

pim_inline float4 VEC_CALL Clamp_c32(
    R8G8B8A8_t const *const pim_noalias buffer, int2 size, int2 coord)
{
    i32 index = Clamp(size, coord);
    return GammaDecode_rgba8(buffer[index]);
}
pim_inline float4 VEC_CALL Wrap_c32(
    R8G8B8A8_t const *const pim_noalias buffer, int2 size, int2 coord)
{
    i32 index = Wrap(size, coord);
    return GammaDecode_rgba8(buffer[index]);
}
pim_inline float4 VEC_CALL WrapPow2_c32(
    R8G8B8A8_t const *const pim_noalias buffer, int2 size, int2 coord)
{
    i32 index = WrapPow2(size, coord);
    return GammaDecode_rgba8(buffer[index]);
}

// ----------------------------------------------------------------------------

pim_inline float4 VEC_CALL UvClamp_c32(
    R8G8B8A8_t const *const pim_noalias buffer, int2 size, float2 uv)
{
    i32 index = UvClamp(size, uv);
    return GammaDecode_rgba8(buffer[index]);
}
pim_inline float4 VEC_CALL UvWrap_c32(
    R8G8B8A8_t const *const pim_noalias buffer, int2 size, float2 uv)
{
    i32 index = UvWrap(size, uv);
    return GammaDecode_rgba8(buffer[index]);
}
pim_inline float4 VEC_CALL UvWrapPow2_c32(
    R8G8B8A8_t const *const pim_noalias buffer, int2 size, float2 uv)
{
    i32 index = UvWrapPow2(size, uv);
    return GammaDecode_rgba8(buffer[index]);
}

// ----------------------------------------------------------------------------

pim_inline float4 VEC_CALL Clamp_dir8(
    R8G8B8A8_t const *const pim_noalias buffer, int2 size, int2 coord)
{
    i32 index = Clamp(size, coord);
    return ColorToDirection(buffer[index]);
}
pim_inline float4 VEC_CALL Wrap_dir8(
    R8G8B8A8_t const *const pim_noalias buffer, int2 size, int2 coord)
{
    i32 index = Wrap(size, coord);
    return ColorToDirection(buffer[index]);
}
pim_inline float4 VEC_CALL WrapPow2_dir8(
    R8G8B8A8_t const *const pim_noalias buffer, int2 size, int2 coord)
{
    i32 index = WrapPow2(size, coord);
    return ColorToDirection(buffer[index]);
}

// ----------------------------------------------------------------------------

pim_inline float4 VEC_CALL UvClamp_dir8(
    R8G8B8A8_t const *const pim_noalias buffer, int2 size, float2 uv)
{
    i32 index = UvClamp(size, uv);
    return ColorToDirection(buffer[index]);
}
pim_inline float4 VEC_CALL UvWrap_dir8(
    R8G8B8A8_t const *const pim_noalias buffer, int2 size, float2 uv)
{
    i32 index = UvWrap(size, uv);
    return ColorToDirection(buffer[index]);
}
pim_inline float4 VEC_CALL UvWrapPow2_dir8(
    R8G8B8A8_t const *const pim_noalias buffer, int2 size, float2 uv)
{
    i32 index = UvWrapPow2(size, uv);
    return ColorToDirection(buffer[index]);
}

// ----------------------------------------------------------------------------

pim_inline float4 VEC_CALL Clamp_xy16(
    short2 const *const pim_noalias buffer, int2 size, int2 coord)
{
    i32 index = Clamp(size, coord);
    return Xy16ToNormalTs(buffer[index]);
}
pim_inline float4 VEC_CALL Wrap_xy16(
    short2 const *const pim_noalias buffer, int2 size, int2 coord)
{
    i32 index = Wrap(size, coord);
    return Xy16ToNormalTs(buffer[index]);
}
pim_inline float4 VEC_CALL WrapPow2_xy16(
    short2 const *const pim_noalias buffer, int2 size, int2 coord)
{
    i32 index = WrapPow2(size, coord);
    return Xy16ToNormalTs(buffer[index]);
}

// ----------------------------------------------------------------------------

pim_inline float4 VEC_CALL UvClamp_xy16(
    short2 const *const pim_noalias buffer, int2 size, float2 uv)
{
    i32 index = UvClamp(size, uv);
    return Xy16ToNormalTs(buffer[index]);
}
pim_inline float4 VEC_CALL UvWrap_xy16(
    short2 const *const pim_noalias buffer, int2 size, float2 uv)
{
    i32 index = UvWrap(size, uv);
    return Xy16ToNormalTs(buffer[index]);
}
pim_inline float4 VEC_CALL UvWrapPow2_xy16(
    short2 const *const pim_noalias buffer, int2 size, float2 uv)
{
    i32 index = UvWrapPow2(size, uv);
    return Xy16ToNormalTs(buffer[index]);
}

// ----------------------------------------------------------------------------

typedef struct bilinear_s
{
    i32 a;
    i32 b;
    i32 c;
    i32 d;
    float2 frac;
} bilinear_t;

// ----------------------------------------------------------------------------

pim_inline bilinear_t VEC_CALL BilinearClamp(int2 size, float2 uv)
{
    bilinear_t b;
    float2 coordf = UvToCoordf(size, uv);
    b.frac = f2_frac(coordf);
    int2 a = f2_i2(coordf);
    b.a = Clamp(size, a);
    b.b = Clamp(size, i2_v(a.x + 1, a.y + 0));
    b.c = Clamp(size, i2_v(a.x + 0, a.y + 1));
    b.d = Clamp(size, i2_v(a.x + 1, a.y + 1));
    return b;
}
pim_inline bilinear_t VEC_CALL BilinearWrap(int2 size, float2 uv)
{
    bilinear_t b;
    float2 coordf = UvToCoordf(size, uv);
    b.frac = f2_frac(coordf);
    int2 a = f2_i2(coordf);
    b.a = Wrap(size, a);
    b.b = Wrap(size, i2_v(a.x + 1, a.y + 0));
    b.c = Wrap(size, i2_v(a.x + 0, a.y + 1));
    b.d = Wrap(size, i2_v(a.x + 1, a.y + 1));
    return b;
}
pim_inline bilinear_t VEC_CALL BilinearWrapPow2(int2 size, float2 uv)
{
    bilinear_t b;
    float2 coordf = UvToCoordf(size, uv);
    b.frac = f2_frac(coordf);
    int2 a = f2_i2(coordf);
    b.a = WrapPow2(size, a);
    b.b = WrapPow2(size, i2_v(a.x + 1, a.y + 0));
    b.c = WrapPow2(size, i2_v(a.x + 0, a.y + 1));
    b.d = WrapPow2(size, i2_v(a.x + 1, a.y + 1));
    return b;
}

// ----------------------------------------------------------------------------

pim_inline float4 VEC_CALL BilinearBlend_c32(
    R8G8B8A8_t a, R8G8B8A8_t b, R8G8B8A8_t c, R8G8B8A8_t d,
    float2 frac)
{
    float4 af = GammaDecode_rgba8(a);
    float4 bf = GammaDecode_rgba8(b);
    float4 cf = GammaDecode_rgba8(c);
    float4 df = GammaDecode_rgba8(d);
    return f4_bilerp(af, bf, cf, df, frac);
}
pim_inline float4 VEC_CALL BilinearBlend_dir8(
    R8G8B8A8_t a, R8G8B8A8_t b, R8G8B8A8_t c, R8G8B8A8_t d,
    float2 frac)
{
    float4 af = ColorToDirection(a);
    float4 bf = ColorToDirection(b);
    float4 cf = ColorToDirection(c);
    float4 df = ColorToDirection(d);
    float4 res = f4_bilerp(af, bf, cf, df, frac);
    return f4_normalize3(res);
}
pim_inline float4 VEC_CALL BilinearBlend_xy16(
    short2 a, short2 b, short2 c, short2 d,
    float2 frac)
{
    float4 af = Xy16ToNormalTs(a);
    float4 bf = Xy16ToNormalTs(b);
    float4 cf = Xy16ToNormalTs(c);
    float4 df = Xy16ToNormalTs(d);
    float4 res = f4_bilerp(af, bf, cf, df, frac);
    return f4_normalize3(res);
}

// ----------------------------------------------------------------------------

pim_inline float4 VEC_CALL Bilinear_f4(
    float4 const *const pim_noalias buffer, int2 size, bilinear_t bi)
{
    return f4_bilerp(
        buffer[bi.a],
        buffer[bi.b],
        buffer[bi.c],
        buffer[bi.d],
        bi.frac);
}
pim_inline float3 VEC_CALL Bilinear_f3(
    float3 const *const pim_noalias buffer, int2 size, bilinear_t bi)
{
    return f3_bilerp(
        buffer[bi.a],
        buffer[bi.b],
        buffer[bi.c],
        buffer[bi.d],
        bi.frac);
}
pim_inline float2 VEC_CALL Bilinear_f2(
    float2 const *const pim_noalias buffer, int2 size, bilinear_t bi)
{
    return f2_bilerp(
        buffer[bi.a],
        buffer[bi.b],
        buffer[bi.c],
        buffer[bi.d],
        bi.frac);
}
pim_inline float VEC_CALL Bilinear_f1(
    float const *const pim_noalias buffer, int2 size, bilinear_t bi)
{
    return f1_bilerp(
        buffer[bi.a],
        buffer[bi.b],
        buffer[bi.c],
        buffer[bi.d],
        bi.frac.x, bi.frac.y);
}
pim_inline float4 VEC_CALL Bilinear_c32(
    R8G8B8A8_t const *const pim_noalias buffer, int2 size, bilinear_t bi)
{
    return BilinearBlend_c32(
        buffer[bi.a],
        buffer[bi.b],
        buffer[bi.c],
        buffer[bi.d],
        bi.frac);
}
pim_inline float4 VEC_CALL Bilinear_dir8(
    R8G8B8A8_t const *const pim_noalias buffer, int2 size, bilinear_t bi)
{
    return BilinearBlend_dir8(
        buffer[bi.a],
        buffer[bi.b],
        buffer[bi.c],
        buffer[bi.d],
        bi.frac);
}
pim_inline float4 VEC_CALL Bilinear_xy16(
    short2 const *const pim_noalias buffer, int2 size, bilinear_t bi)
{
    return BilinearBlend_xy16(
        buffer[bi.a],
        buffer[bi.b],
        buffer[bi.c],
        buffer[bi.d],
        bi.frac);
}

// ----------------------------------------------------------------------------

pim_inline float4 VEC_CALL UvBilinearClamp_f4(
    float4 const *const pim_noalias buffer, int2 size, float2 uv)
{
    return Bilinear_f4(buffer, size, BilinearClamp(size, uv));
}
pim_inline float4 VEC_CALL UvBilinearWrap_f4(
    float4 const *const pim_noalias buffer, int2 size, float2 uv)
{
    return Bilinear_f4(buffer, size, BilinearWrap(size, uv));
}
pim_inline float3 VEC_CALL UvBilinearClamp_f3(
    float3 const *const pim_noalias buffer, int2 size, float2 uv)
{
    return Bilinear_f3(buffer, size, BilinearClamp(size, uv));
}
pim_inline float3 VEC_CALL UvBilinearWrap_f3(
    float3 const *const pim_noalias buffer, int2 size, float2 uv)
{
    return Bilinear_f3(buffer, size, BilinearWrap(size, uv));
}
pim_inline float2 VEC_CALL UvBilinearClamp_f2(
    float2 const *const pim_noalias buffer, int2 size, float2 uv)
{
    return Bilinear_f2(buffer, size, BilinearClamp(size, uv));
}
pim_inline float2 VEC_CALL UvBilinearWrap_f2(
    float2 const *const pim_noalias buffer, int2 size, float2 uv)
{
    return Bilinear_f2(buffer, size, BilinearWrap(size, uv));
}
pim_inline float VEC_CALL UvBilinearClamp_f1(
    float const *const pim_noalias buffer, int2 size, float2 uv)
{
    return Bilinear_f1(buffer, size, BilinearClamp(size, uv));
}
pim_inline float VEC_CALL UvBilinearWrap_f1(
    float const *const pim_noalias buffer, int2 size, float2 uv)
{
    return Bilinear_f1(buffer, size, BilinearWrap(size, uv));
}

// ----------------------------------------------------------------------------

pim_inline float4 VEC_CALL UvBilinearClamp_c32(
    R8G8B8A8_t const *const pim_noalias buffer, int2 size, float2 uv)
{
    return Bilinear_c32(buffer, size, BilinearClamp(size, uv));
}
pim_inline float4 VEC_CALL UvBilinearWrap_c32(
    R8G8B8A8_t const *const pim_noalias buffer, int2 size, float2 uv)
{
    return Bilinear_c32(buffer, size, BilinearWrap(size, uv));
}
pim_inline float4 VEC_CALL UvBilinearWrapPow2_c32(
    R8G8B8A8_t const *const pim_noalias buffer, int2 size, float2 uv)
{
    return Bilinear_c32(buffer, size, BilinearWrapPow2(size, uv));
}

// ----------------------------------------------------------------------------

pim_inline float4 VEC_CALL UvBilinearClamp_dir8(
    R8G8B8A8_t const *const pim_noalias buffer, int2 size, float2 uv)
{
    return Bilinear_dir8(buffer, size, BilinearClamp(size, uv));
}
pim_inline float4 VEC_CALL UvBilinearWrap_dir8(
    R8G8B8A8_t const *const pim_noalias buffer, int2 size, float2 uv)
{
    return Bilinear_dir8(buffer, size, BilinearWrap(size, uv));
}
pim_inline float4 VEC_CALL UvBilinearWrapPow2_dir8(
    R8G8B8A8_t const *const pim_noalias buffer, int2 size, float2 uv)
{
    return Bilinear_dir8(buffer, size, BilinearWrapPow2(size, uv));
}

// ----------------------------------------------------------------------------

pim_inline float4 VEC_CALL UvBilinearClamp_xy16(
    short2 const *const pim_noalias buffer, int2 size, float2 uv)
{
    return Bilinear_xy16(buffer, size, BilinearClamp(size, uv));
}
pim_inline float4 VEC_CALL UvBilinearWrap_xy16(
    short2 const *const pim_noalias buffer, int2 size, float2 uv)
{
    return Bilinear_xy16(buffer, size, BilinearWrap(size, uv));
}
pim_inline float4 VEC_CALL UvBilinearWrapPow2_xy16(
    short2 const *const pim_noalias buffer, int2 size, float2 uv)
{
    return Bilinear_xy16(buffer, size, BilinearWrapPow2(size, uv));
}

// ----------------------------------------------------------------------------

pim_inline float4 VEC_CALL TrilinearClamp_f4(
    float4 const *const pim_noalias buffer,
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
    float4 const *const pim_noalias buffer,
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

pim_inline float3 VEC_CALL TrilinearClamp_f3(
    float3 const *const pim_noalias buffer,
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

    float3 s0 = UvBilinearClamp_f3(buffer + i0, size0, uv);
    float3 s1 = UvBilinearClamp_f3(buffer + i1, size1, uv);

    return f3_lerpvs(s0, s1, mfrac);
}
pim_inline float3 VEC_CALL TrilinearWrap_f3(
    float3 const *const pim_noalias buffer,
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

    float3 s0 = UvBilinearWrap_f3(buffer + i0, size0, uv);
    float3 s1 = UvBilinearWrap_f3(buffer + i1, size1, uv);

    return f3_lerpvs(s0, s1, mfrac);
}

pim_inline float2 VEC_CALL TrilinearClamp_f2(
    float2 const *const pim_noalias buffer,
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

    float2 s0 = UvBilinearClamp_f2(buffer + i0, size0, uv);
    float2 s1 = UvBilinearClamp_f2(buffer + i1, size1, uv);

    return f2_lerpvs(s0, s1, mfrac);
}
pim_inline float2 VEC_CALL TrilinearWrap_f2(
    float2 const *const pim_noalias buffer,
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

    float2 s0 = UvBilinearWrap_f2(buffer + i0, size0, uv);
    float2 s1 = UvBilinearWrap_f2(buffer + i1, size1, uv);

    return f2_lerpvs(s0, s1, mfrac);
}

pim_inline float VEC_CALL TrilinearClamp_f1(
    float const *const pim_noalias buffer,
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

    float s0 = UvBilinearClamp_f1(buffer + i0, size0, uv);
    float s1 = UvBilinearClamp_f1(buffer + i1, size1, uv);

    return f1_lerp(s0, s1, mfrac);
}
pim_inline float VEC_CALL TrilinearWrap_f1(
    float const *const pim_noalias buffer,
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

    float s0 = UvBilinearWrap_f1(buffer + i0, size0, uv);
    float s1 = UvBilinearWrap_f1(buffer + i1, size1, uv);

    return f1_lerp(s0, s1, mfrac);
}

pim_inline float4 VEC_CALL TrilinearClamp_c32(
    R8G8B8A8_t const *const pim_noalias buffer,
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
    R8G8B8A8_t const *const pim_noalias buffer,
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

pim_inline float4 VEC_CALL TrilinearClamp_dir8(
    R8G8B8A8_t const *const pim_noalias buffer,
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

    float4 s0 = UvBilinearClamp_dir8(buffer + i0, size0, uv);
    float4 s1 = UvBilinearClamp_dir8(buffer + i1, size1, uv);

    float4 N = f4_lerpvs(s0, s1, mfrac);
    return f4_normalize3(N);
}
pim_inline float4 VEC_CALL TrilinearWrap_dir8(
    R8G8B8A8_t const *const pim_noalias buffer,
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

    float4 s0 = UvBilinearWrap_dir8(buffer + i0, size0, uv);
    float4 s1 = UvBilinearWrap_dir8(buffer + i1, size1, uv);

    float4 N = f4_lerpvs(s0, s1, mfrac);
    return f4_normalize3(N);
}

pim_inline void VEC_CALL Write_f4(
    float4 *const pim_noalias dst, int2 size, int2 coord, float4 src)
{
    i32 i = Clamp(size, coord);
    dst[i] = src;
}

pim_inline void VEC_CALL Write_c32(
    R8G8B8A8_t *const pim_noalias dst, int2 size, int2 coord, float4 src)
{
    i32 i = Clamp(size, coord);
    dst[i] = GammaEncode_rgba8(src);
}

PIM_C_END
