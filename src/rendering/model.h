#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct mmodel_s mmodel_t;
typedef struct drawables_s drawables_t;

void ModelToDrawables(mmodel_t const *const model, drawables_t *const dr);
bool LoadModelAsDrawables(const char* name, drawables_t *const dr, bool loadlights);
void LoadProgs(mmodel_t const *const model, bool loadlights);

PIM_C_END
