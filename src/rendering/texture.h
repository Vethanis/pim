#pragma once

#include "common/macro.h"
#include "math/types.h"

PIM_C_BEGIN

typedef struct textureid_s
{
    u32 index : 24;
    u32 version : 8;
} textureid_t;

typedef struct texture_s
{
    int2 size;
    float4* pim_noalias texels;
} texture_t;

void texture_sys_init(void);
void texture_sys_update(void);
void texture_sys_shutdown(void);

void texture_sys_gui(bool* pEnabled);

bool texture_load(const char* path, textureid_t* idOut);
bool texture_new(texture_t* tex, const char* name, textureid_t* idOut);

bool texture_exists(textureid_t id);

void texture_retain(textureid_t id);
void texture_release(textureid_t id);

bool texture_get(textureid_t id, texture_t* dst);
bool texture_set(textureid_t id, texture_t* src);

bool texture_find(const char* name, textureid_t* idOut);

bool texture_unpalette(
    const u8* bytes,
    int2 size,
    const char* name,
    textureid_t* albedoOut,
    textureid_t* romeOut,
    textureid_t* normalOut);

PIM_C_END
