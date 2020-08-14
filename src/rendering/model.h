#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct mmodel_s mmodel_t;

void ModelToDrawables(const mmodel_t* model);
bool LoadModelAsDrawables(const char* name, bool loadlights);
void LoadProgs(const mmodel_t* model, bool loadlights);

PIM_C_END
