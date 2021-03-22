#include "rendering/tonemap.h"
#include "math/color.h"

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
