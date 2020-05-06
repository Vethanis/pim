#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "rendering/mesh.h"
#include "rendering/material.h"

void SubmitMesh(meshid_t worldMesh, material_t material);

void FragmentStage_Init(void);
void FragmentStage_Shutdown(void);
struct task_s* FragmentStage(struct framebuf_s* target);

// temporary until we have a better scene data structure
float3* LightDir(void);
float3* LightRad(void);
float3* DiffuseGI(void);
float3* SpecularGI(void);
u64 Frag_TrisCulled(void);
u64 Frag_TrisDrawn(void);

PIM_C_END
