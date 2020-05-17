#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "rendering/mesh.h"
#include "rendering/material.h"

void SubmitMesh(meshid_t worldMesh, material_t material);

void FragmentStage_Init(void);
void FragmentStage_Shutdown(void);
struct task_s* FragmentStage(struct framebuf_s* target);

SG_t* SG_Get(void);
i32 SG_GetCount(void);
void SG_SetCount(i32 ct);
float4* DiffuseGI(void);
float4* SpecularGI(void);
u64 Frag_TrisCulled(void);
u64 Frag_TrisDrawn(void);

PIM_C_END
