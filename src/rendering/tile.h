#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "rendering/constants.h"
#include "rendering/framebuffer.h"
#include "math/float2_funcs.h"
#include "math/int2_funcs.h"

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

pim_inline float VEC_CALL TileDepth(const framebuf_t* buf, int2 tile)
{
    i32 w = buf->width;
    i32 h = buf->height;
    i32 m = 0;
    while ((w*h) >= (kTileCount * 2))
    {
        ++m;
        w = w >> 1;
        h = h >> 1;
    }
    ASSERT(m < buf->mipCount);
    i32 offset = buf->offsets[m];
    tile.x = tile.x >> m;
    tile.y = tile.y >> m;
    i32 i = IndexTile(tile, kDrawWidth >> m, 0, 0);
    ASSERT(i >= 0);
    ASSERT(i < (w*h));
    return buf->depth[offset + i];
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
    const float2 kScale = { kDrawWidth, kDrawHeight };
    s = f2_mul(f2_unorm(s), kScale);
    s = f2_subvs(s, 0.5f); // pixel center offset
    int2 i2 = f2_i2(s); // TODO: clamp to within tile. data race when s underflows tile.
    i32 i = i2.x + i2.y * kDrawWidth;
    ASSERT((u32)i < (u32)kDrawPixels);
    return i;
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

PIM_C_END
