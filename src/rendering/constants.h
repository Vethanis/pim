#pragma once

#include "common/macro.h"

PIM_C_BEGIN

enum
{
    kDrawWidth = 320 << 0,
    kDrawHeight = 240 << 0,
    kDrawPixels = kDrawWidth * kDrawHeight,
    kTilesPerDim = 8,
    kTileCount = kTilesPerDim * kTilesPerDim,
    kTileWidth = kDrawWidth / kTilesPerDim,
    kTileHeight = kDrawHeight / kTilesPerDim,
    kTilePixels = kTileWidth * kTileHeight,
};
SASSERT((kDrawWidth % kTilesPerDim) == 0);
SASSERT((kDrawHeight % kTilesPerDim) == 0);

static const float kDrawAspect = (float)kDrawWidth / kDrawHeight;

PIM_C_END
