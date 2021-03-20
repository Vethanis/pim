#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct mmodel_s mmodel_t;
typedef struct Entities_s Entities;

void model_sys_init(void);
void model_sys_update(void);
void model_sys_shutdown(void);

void ModelToDrawables(mmodel_t const *const model, Entities *const dr);
bool LoadModelAsDrawables(const char* name, Entities *const dr, bool loadlights);
void LoadProgs(mmodel_t const *const model, bool loadlights);

PIM_C_END
