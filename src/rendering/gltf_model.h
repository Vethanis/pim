#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct drawables_s drawables_t;

bool gltf_model_load(const char* path, drawables_t* dst);

PIM_C_END
