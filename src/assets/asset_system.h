#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct asset_s
{
    i32 length;
    const void* pData;
} asset_t;

void asset_sys_init(void);
void asset_sys_update(void);
void asset_sys_shutdown(void);

bool asset_get(const char* name, asset_t* assetOut);

void asset_gui(void);

PIM_C_END
