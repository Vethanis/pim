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

pim_inline int2 VEC_CALL EncodeCoord2(int2 size, i32 i)
{
    int2 coord;
    coord.x = i % size.x;
    coord.y = i / size.x;
    return coord;
}
pim_inline i32 VEC_CALL DecodeCoord2(int2 size, int2 coord)
{
    return size.x * coord.y + coord.x;
}

pim_inline int3 VEC_CALL EncodeCoord3(int3 size, i32 i)
{
    int3 coord;
    coord.x = i % size.x;
    coord.y = (i / size.x) % size.y;
    coord.z = i / (size.x * size.y);
    return coord;
}
pim_inline i32 VEC_CALL DecodeCoord3(int3 size, int3 coord)
{
    return (size.x * size.y) * coord.z + size.x * coord.y + coord.x;
}

// ----------------------------------------------------------------------------

pim_inline i32 VEC_CALL PointClamp1D(i32 size, float u)
{
    return (i32)(f1_sat(u) * (size - 1) + 0.5f);
}
pim_inline i32 VEC_CALL PointWrap1D(i32 size, float u)
{
    u = (u >= 0.0f) ? u : (1.0f - u);
    u = f1_frac(u);
    return PointClamp1D(size, u);
}
pim_inline float VEC_CALL PointInvert1D(i32 size, i32 x)
{
    return (x + 0.5f) / size;
}

pim_inline int2 VEC_CALL PointClamp2D(int2 size, float2 uv)
{
    int2 coord;
    coord.x = PointClamp1D(size.x, uv.x);
    coord.y = PointClamp1D(size.y, uv.y);
    return coord;
}
pim_inline int2 VEC_CALL PointWrap2D(int2 size, float2 uv)
{
    int2 coord;
    coord.x = PointWrap1D(size.x, uv.x);
    coord.y = PointWrap1D(size.y, uv.y);
    return coord;
}
pim_inline float2 VEC_CALL PointInvert2D(int2 size, int2 coord)
{
    float2 uv;
    uv.x = PointInvert1D(size.x, coord.x);
    uv.y = PointInvert1D(size.y, coord.y);
    return uv;
}

pim_inline int3 VEC_CALL PointClamp3D(int3 size, float3 uvw)
{
    int3 coord;
    coord.x = PointClamp1D(size.x, uvw.x);
    coord.y = PointClamp1D(size.y, uvw.y);
    coord.z = PointClamp1D(size.z, uvw.z);
    return coord;
}
pim_inline int3 VEC_CALL PointWrap3D(int3 size, float3 uvw)
{
    int3 coord;
    coord.x = PointWrap1D(size.x, uvw.x);
    coord.y = PointWrap1D(size.y, uvw.y);
    coord.z = PointWrap1D(size.z, uvw.z);
    return coord;
}
pim_inline float3 VEC_CALL PointInvert3D(int3 size, int3 coord)
{
    float3 uvw;
    uvw.x = PointInvert1D(size.x, coord.x);
    uvw.y = PointInvert1D(size.y, coord.y);
    uvw.z = PointInvert1D(size.z, coord.z);
    return uvw;
}

// ----------------------------------------------------------------------------

typedef struct linear_s
{
    i32 a;
    i32 b;
    float t;
} linear_t;

pim_inline linear_t VEC_CALL LinearClamp(i32 size, float u)
{
    float x = f1_sat(u) * (size - 1);
    linear_t f;
    f.a = (i32)x;
    f.t = x - (float)f.a;
    f.b = i1_min(f.a + 1, (size - 1));
    return f;
}
pim_inline linear_t VEC_CALL LinearWrap(i32 size, float u)
{
    u = (u >= 0.0f) ? u : (1.0f - u);
    u = f1_frac(u);
    return LinearClamp(size, u);
}

pim_inline float VEC_CALL LinearInvert(i32 size, i32 x)
{
    return (float)x / (float)(size - 1);
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
    linear_t x = LinearClamp(size.x, uv.x);
    linear_t y = LinearClamp(size.y, uv.y);
    b.frac.x = x.t;
    b.frac.y = y.t;
    b.a = y.a * size.x + x.a;
    b.b = y.a * size.x + x.b;
    b.c = y.b * size.x + x.a;
    b.d = y.b * size.x + x.b;
    return b;
}
pim_inline bilinear_t VEC_CALL BilinearWrap(int2 size, float2 uv)
{
    uv.x = (uv.x >= 0.0f) ? uv.x : (1.0f - uv.x);
    uv.y = (uv.y >= 0.0f) ? uv.y : (1.0f - uv.y);
    uv = f2_frac(uv);
    return BilinearClamp(size, uv);
}
pim_inline float2 VEC_CALL BilinearInvert(int2 size, int2 coord)
{
    float2 uv;
    uv.x = LinearInvert(size.x, coord.x);
    uv.y = LinearInvert(size.y, coord.y);
    return uv;
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
pim_inline float4 VEC_CALL BilinearBlend_xy16(
    short2 a, short2 b, short2 c, short2 d,
    float2 frac)
{
    const float s = (1.0f / (1 << 15));
    float2 af = { a.x * s, a.y * s };
    float2 bf = { b.x * s, b.y * s };
    float2 cf = { c.x * s, c.y * s };
    float2 df = { d.x * s, d.y * s };
    float2 xy = f2_bilerp(af, bf, cf, df, frac);
    float z = sqrtf(f1_max(kEpsilon, 1.0f - f2_dot(xy, xy)));
    return f4_v(xy.x, xy.y, z, 0.0f);
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

pim_inline void VEC_CALL Write_f4(
    float4 *const pim_noalias dst, int2 size, int2 coord, float4 src)
{
    i32 i = size.x * coord.y + coord.x;
    dst[i] = src;
}

pim_inline void VEC_CALL Write_c32(
    R8G8B8A8_t *const pim_noalias dst, int2 size, int2 coord, float4 src)
{
    i32 i = size.x * coord.y + coord.x;
    dst[i] = GammaEncode_rgba8(src);
}

PIM_C_END
