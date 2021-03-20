#pragma once

#include "common/macro.h"
#include "math/types.h"
#include "common/guid.h"
#include "common/dbytes.h"
#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

typedef struct Table_s Table;
typedef struct Crate_s Crate;
typedef struct Material_s Material;

typedef struct TextureId_s
{
    u32 version : 8;
    u32 index : 24;
} TextureId;

typedef struct DiskTextureId_s
{
    Guid id;
} DiskTextureId;

typedef struct Texture_s
{
    int2 size;
    void* pim_noalias texels;
    VkFormat format;
    vkrTextureId slot;
} Texture;

#define kTextureVersion 5
typedef struct DiskTexture_s
{
    i32 version;
    VkFormat format;
    char name[64];
    int2 size;
} DiskTexture;

Table const *const texture_table(void);

void texture_sys_init(void);
void texture_sys_update(void);
void texture_sys_shutdown(void);
void texture_sys_gui(bool* pEnabled);

bool texture_loadat(const char* path, VkFormat format, TextureId* idOut);
bool texture_new(Texture* tex, VkFormat format, Guid name, TextureId* idOut);

bool texture_exists(TextureId id);

void texture_retain(TextureId id);
void texture_release(TextureId id);

Texture* texture_get(TextureId id);

bool texture_find(Guid name, TextureId* idOut);
bool texture_getname(TextureId id, Guid* nameOut);
bool texture_getnamestr(TextureId tid, char* dst, i32 size);

bool texture_save(Crate* crate, TextureId tid, Guid* dst);
bool texture_load(Crate* crate, Guid name, TextureId* dst);

bool texture_unpalette(
    u8 const *const pim_noalias bytes,
    int2 size,
    const char* name,
    Material const *const material,
    float4 flatRome,
    TextureId *const albedoOut,
    TextureId *const romeOut,
    TextureId *const normalOut);

PIM_C_END
