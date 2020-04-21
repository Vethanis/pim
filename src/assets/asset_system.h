#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct asset_s
{
    i32 offset;
    i32 length;
    const void* pData;
    const char* name;
} asset_t;

void asset_sys_init(void);
void asset_sys_update(void);
void asset_sys_shutdown(void);

void asset_gui(void);

bool asset_sys_get(const char* name, asset_t* assetOut);

PIM_C_END
