#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "rendering/texture.h"

typedef struct material_s
{
    texture_t* albedo;
    texture_t* omre; // occlusion, metallic, roughness, emission
} material_t;

PIM_C_END
