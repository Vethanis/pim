#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include <stdint.h>

int32_t env_errno(void);

void pim_searchenv(const char* filename, const char* varname, char* dst);
const char* pim_getenv(const char* varname);
void pim_putenv(const char* varname, const char* value);

PIM_C_END
