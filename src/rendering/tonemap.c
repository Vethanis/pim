#include "rendering/tonemap.h"
#include "math/color.h"

static float4 VEC_CALL TMapReinhard(float4 color, float4 params)
{
    return tmap4_reinhard(color);
}

static float4 VEC_CALL TMapUncharted2(float4 color, float4 params)
{
    return tmap4_uchart2(color);
}

static float4 VEC_CALL TMapFilmicAlu(float4 color, float4 params)
{
    return tmap4_filmic(color);
}

static float4 VEC_CALL TMapAces(float4 color, float4 params)
{
    return tmap4_aces(color);
}

static const TonemapFn kTonemappers[] =
{
    TMapReinhard,
    TMapUncharted2,
    tmap4_hable,
    TMapFilmicAlu,
    TMapAces,
};
SASSERT(NELEM(kTonemappers) == TMap_COUNT);

static const char* const kTonemapNames[] =
{
    "Reinhard",
    "Uncharted2",
    "Hable",
    "Filmic",
    "Aces",
};
SASSERT(NELEM(kTonemapNames) == TMap_COUNT);

const char* const* Tonemap_Names(void)
{
    return kTonemapNames;
}

TonemapFn Tonemap_GetFunc(TonemapId id)
{
    ASSERT(id >= 0);
    ASSERT(id < TMap_COUNT);
    return kTonemappers[id];
}

static const float4 kDefaultParams = { 0.15f, 0.5f, 0.1f, 0.2f };
float4 Tonemap_DefParams(void)
{
    return kDefaultParams;
}
