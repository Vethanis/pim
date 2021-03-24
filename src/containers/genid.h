#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct hdl_s GenId;
typedef union { u32 num; GenId id; } GenId_u;

PIM_C_END
