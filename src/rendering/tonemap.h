#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef enum
{
    TMap_Reinhard = 0,
    TMap_Uncharted2,
    TMap_Hable,
    TMap_ACES,

    TMap_COUNT
} TonemapId;
const char* const* Tonemap_Names(void);

PIM_C_END
