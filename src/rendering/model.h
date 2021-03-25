#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct mmodel_s mmodel_t;
typedef struct Entities_s Entities;

void ModelSys_Init(void);
void ModelSys_Update(void);
void ModelSys_Shutdown(void);

void ModelToDrawables(mmodel_t const *const model, Entities *const dr);
bool LoadModelAsDrawables(const char* name, Entities *const dr);
void LoadProgs(mmodel_t const *const model, bool loadlights);

PIM_C_END
