#pragma once

#include "common/macro.h"

PIM_C_BEGIN

enum
{
    kNumFrames = 2,
    kFrameMask = kNumFrames - 1,
    kDrawWidth = 320,
    kDrawHeight = 240,
    kDrawPixels = kDrawWidth * kDrawHeight,
    kTilesPerDim = 8,
    kTileCount = kTilesPerDim * kTilesPerDim,
    kTileWidth = kDrawWidth / kTilesPerDim,
    kTileHeight = kDrawHeight / kTilesPerDim,
    kTilePixels = kTileWidth * kTileHeight,
};

PIM_C_END
