#pragma once

#include "common/macro.h"
#include "math/types.h"
#include "common/guid.h"
#include "common/dbytes.h"
#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

typedef struct table_s table_t;
typedef struct crate_s crate_t;

typedef struct dtextureid_s
{
    guid_t id;
} dtextureid_t;

typedef struct texture_s
{
    int2 size;
    void* pim_noalias texels;
    vkrTexture2D vkrtex;
} texture_t;

#define kTextureVersion 5
typedef struct dtexture_s
{
    i32 version;
    VkFormat format;
    char name[64];
    int2 size;
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

texture_t* texture_get(textureid_t id);

bool texture_find(guid_t name, textureid_t* idOut);
bool texture_getname(textureid_t id, guid_t* nameOut);

bool texture_save(crate_t* crate, textureid_t tid, guid_t* dst);
bool texture_load(crate_t* crate, guid_t name, textureid_t* dst);

bool texture_unpalette(
    const u8* bytes,
    int2 size,
    const char* name,
    u32 matflags,
    float4 flatRome,
    textureid_t* albedoOut,
    textureid_t* romeOut,
    textureid_t* normalOut);

PIM_C_END
