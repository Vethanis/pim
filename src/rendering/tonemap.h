#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "math/types.h"

typedef enum
{
    TMap_Reinhard = 0,
    TMap_Uncharted2,
    TMap_Hable,
    TMap_Filmic,
    TMap_ACES,

    TMap_COUNT
} TonemapId;

typedef float4(VEC_CALL *TonemapFn)(float4 color, float4 params);

const char* const* Tonemap_Names(void);
TonemapFn Tonemap_GetFunc(TonemapId id);
float4 Tonemap_DefParams(void);

PIM_C_END
