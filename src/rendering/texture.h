#pragma once

#include "common/macro.h"
#include "math/types.h"

PIM_C_BEGIN

typedef struct texture_s
{
    int2 size;
    u32* pim_noalias texels;
} texture_t;

typedef struct textureid_s
{
    i32 index;
    i32 version;
} textureid_t;

textureid_t texture_load(const char* path);
textureid_t texture_new(int2 size, u32* texels, const char* name);

bool texture_exists(textureid_t id);

void texture_retain(textureid_t id);
void texture_release(textureid_t id);

bool texture_get(textureid_t id, texture_t* dst);
bool texture_set(textureid_t id, int2 size, u32* texels);

textureid_t texture_lookup(const char* name);

textureid_t texture_unpalette(const u8* bytes, int2 size, const char* name);
textureid_t texture_lumtonormal(textureid_t src, float scale, const char* name);

PIM_C_END
