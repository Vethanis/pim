#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "rendering/mesh.h"
#include "rendering/material.h"

struct task_s* FragmentStage(struct framebuf_s* target, meshid_t worldMesh, material_t material);

// temporary until we have a better scene data structure
float3* LightDir(void);
float3* LightRad(void);
float3* DiffuseGI(void);
float3* SpecularGI(void);

PIM_C_END
