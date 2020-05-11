#pragma once

#include "common/macro.h"

PIM_C_BEGIN

bool igTableHeader(i32 count, const char* const* titles, i32* pSelection);
void igTableFooter(void);

PIM_C_END
