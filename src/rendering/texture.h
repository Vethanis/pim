#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "math/types.h"

typedef struct texture_s
{
    u64 version;
    int2 size;
    u32* pim_noalias texels;
} texture_t;

typedef struct textureid_s
{
    u64 version;
    void* handle;
} textureid_t;

textureid_t texture_load(const char* path);
textureid_t texture_create(texture_t* src);
bool texture_destroy(textureid_t id);
bool texture_current(textureid_t id);
bool texture_get(textureid_t id, texture_t* dst);

PIM_C_END
