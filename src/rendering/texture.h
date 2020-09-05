#pragma once

#include "common/macro.h"
#include "math/types.h"
#include "common/guid.h"
#include "common/dbytes.h"
#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

typedef struct table_s table_t;

typedef struct textureid_s
{
    u32 index : 24;
    u32 version : 8;
} textureid_t;

typedef struct dtextureid_s
{
    guid_t id;
} dtextureid_t;

typedef struct texture_s
{
    int2 size;
    u32* pim_noalias texels;
    vkrTexture2D vkrtex;
} texture_t;

#define kTextureVersion 3
typedef struct dtexture_s
{
    i32 version;
    VkFormat format;
    int2 size;
    dbytes_t texels;
} dtexture_t;

const table_t* texture_table(void);

void texture_sys_init(void);
void texture_sys_update(void);
void texture_sys_shutdown(void);
// free vulkan resources
void texture_sys_vkfree(void);
void texture_sys_gui(bool* pEnabled);

bool texture_loadat(const char* path, VkFormat format, textureid_t* idOut);
bool texture_new(texture_t* tex, VkFormat format, guid_t name, textureid_t* idOut);

bool texture_exists(textureid_t id);

void texture_retain(textureid_t id);
void texture_release(textureid_t id);

bool texture_get(textureid_t id, texture_t* dst);
bool texture_set(textureid_t id, texture_t* src);

bool texture_find(guid_t name, textureid_t* idOut);
bool texture_getname(textureid_t id, guid_t* nameOut);

bool texture_save(textureid_t tid, guid_t* dst);
bool texture_load(guid_t name, textureid_t* dst);

bool texture_unpalette(
    const u8* bytes,
    int2 size,
    const char* name,
    textureid_t* albedoOut,
    textureid_t* romeOut,
    textureid_t* normalOut);

PIM_C_END
