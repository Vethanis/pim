#pragma once

#include "common/macro.h"

PIM_C_BEGIN

enum
{
#ifdef _DEBUG
    kDrawWidth = 1920 / 4,
    kDrawHeight = 1080 / 4,
#else
    kDrawWidth = 1920 / 2,
    kDrawHeight = 1080 / 2,
#endif // _DEBUG
    kDrawPixels = kDrawWidth * kDrawHeight,
};

static const float kDrawAspect = (float)kDrawWidth / kDrawHeight;

PIM_C_END
