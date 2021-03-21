#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct asset_s
{
    i32 length;
    const void* pData;
} asset_t;

void AssetSys_Init(void);
void AssetSys_Update(void);
void AssetSys_Shutdown(void);
void AssetSys_Gui(bool* pEnabled);

bool Asset_Get(const char* name, asset_t* assetOut);

PIM_C_END
