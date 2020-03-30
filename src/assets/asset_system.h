#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include <stdint.h>

typedef struct asset_s
{
    const char* name;
    const char* pack;
    int32_t offset;
    int32_t size;
    const void* pData;
} asset_t;

void asset_sys_init(void);
void asset_sys_update(void);
void asset_sys_shutdown(void);

int32_t asset_sys_get(const char* name, asset_t* assetOut);

PIM_C_END
