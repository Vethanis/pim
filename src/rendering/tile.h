#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "rendering/constants.h"
#include "rendering/framebuffer.h"
#include "math/float2_funcs.h"
#include "math/int2_funcs.h"
#include "rendering/sampler.h"

pim_inline int2 VEC_CALL GetTile(i32 i)
{
    i32 x = (i % kTilesPerDim);
    i32 y = (i / kTilesPerDim);
    int2 result = { x * kTileWidth, y * kTileHeight };
    return result;
}

pim_inline i32 VEC_CALL IndexTile(int2 tile, i32 d, i32 x, i32 y)
{
    return (tile.x + x) + (tile.y + y) * d;
}

pim_inline float2 VEC_CALL ScreenToUnorm(int2 screen)
{
    const float2 kRcpScreen = { 1.0f / kDrawWidth, 1.0f / kDrawHeight };
    return f2_mul(i2_f2(screen), kRcpScreen);
}

pim_inline float2 VEC_CALL ScreenToSnorm(int2 screen)
{
    return f2_snorm(ScreenToUnorm(screen));
}

pim_inline i32 VEC_CALL SnormToIndex(float2 s)
{
    const int2 kSize = { kDrawWidth, kDrawHeight };
    return Tex_UvToIndexC(kSize, f2_unorm(s));
}

pim_inline float2 VEC_CALL TileMin(int2 tile)
{
    return ScreenToSnorm(tile);
}

pim_inline float2 VEC_CALL TileMax(int2 tile)
{
    tile.x += kTileWidth;
    tile.y += kTileHeight;
    return ScreenToSnorm(tile);
}

pim_inline float2 VEC_CALL TileCenter(int2 tile)
{
    tile.x += (kTileWidth >> 1);
    tile.y += (kTileHeight >> 1);
    return ScreenToSnorm(tile);
}

pim_inline float VEC_CALL TileDepth(int2 tile, const float* depthBuffer)
{
    const int2 kDrawSize = { kDrawWidth, kDrawHeight };
    const int2 kTileSize = { kTileWidth, kTileHeight };
    const float2 kRcpDrawSize = { 1.0f / kDrawWidth, 1.0f / kDrawHeight };

    const i32 mip = CalcTileMip(kTileSize);
    const i32 mipOffset = CalcMipOffset(kDrawSize, mip);
    const int2 mipSize = CalcMipSize(kDrawSize, mip);
    const float* pim_noalias mipBuffer = depthBuffer + mipOffset;

    float2 uv = f2_addvs(i2_f2(tile), 0.5f);
    uv = f2_mul(uv, kRcpDrawSize);
    i32 i = Tex_UvToIndexC(mipSize, uv);

    return mipBuffer[i];
}

PIM_C_END
