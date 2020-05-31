#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct mmodel_s mmodel_t;

// returns list of drawable ids generated from model
// 0th element is array length
u32* ModelToDrawables(const mmodel_t* model);

// returns list of drawable ids generated from model
// 0th element is array length
u32* LoadModelAsDrawables(const char* name);

PIM_C_END
