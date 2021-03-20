#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct Entities_s Entities;

bool gltf_model_load(const char* path, Entities* dst);

PIM_C_END
