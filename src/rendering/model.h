#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct mmodel_s mmodel_t;

void ModelToDrawables(const mmodel_t* model);
bool LoadModelAsDrawables(const char* name);

PIM_C_END
