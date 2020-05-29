#pragma once

#include "common/macro.h"
#include "components/table.h"
#include "rendering/cubemap.h"

PIM_C_BEGIN

table_t* Cubemaps_New(tables_t* tables);
void Cubemaps_Del(tables_t* tables);
table_t* Cubemaps_Get(tables_t* tables);

ent_t Cubemaps_Add(tables_t* tables, i32 size, float4 pos, float radius);
bool Cubemaps_Rm(tables_t* tables, ent_t ent);

PIM_C_END
