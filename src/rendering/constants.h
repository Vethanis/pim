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
    kTileCount = 8 * 8,
    kTileWidth = kDrawWidth / 8,
    kTileHeight = kDrawHeight / 8,
    kTilePixels = kTileWidth * kTileHeight,
};

PIM_C_END
