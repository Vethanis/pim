#pragma once

#include "common/macro.h"

PIM_C_BEGIN

bool pim_searchenv(const char* filename, const char* varname, char* dst);
const char* pim_getenv(const char* varname);
bool pim_putenv(const char* varname, const char* value);

PIM_C_END
