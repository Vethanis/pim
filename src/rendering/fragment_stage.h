#pragma once

#include "common/macro.h"
#include "math/types.h"

PIM_C_BEGIN

struct task_s* Drawables_Fragment(struct tables_s* tables, struct framebuf_s* frontBuf, const struct framebuf_s* backBuf);

SG_t* SG_Get(void);
float* SG_GetIntegrals(void);
float* SG_GetWeights(void);
i32 SG_GetCount(void);
void SG_SetCount(i32 ct);
float4* DiffuseGI(void);
float4* SpecularGI(void);

PIM_C_END
