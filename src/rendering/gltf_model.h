#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct Entities_s Entities;

bool Gltf_Load(const char* path, Entities* dst);

PIM_C_END
