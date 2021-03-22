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

Table const *const Texture_GetTable(void);

void TextureSys_Init(void);
void TextureSys_Update(void);
void TextureSys_Shutdown(void);
void TextureSys_Gui(bool* pEnabled);

bool Texture_LoadAt(const char* path, VkFormat format, TextureId* idOut);
bool Texture_New(Texture* tex, VkFormat format, Guid name, TextureId* idOut);

bool Texture_Exists(TextureId id);

void Texture_Retain(TextureId id);
void Texture_Release(TextureId id);

Texture* Texture_Get(TextureId id);

bool Texture_Find(Guid name, TextureId* idOut);
bool Texture_GetGuid(TextureId id, Guid* nameOut);
bool Texture_GetName(TextureId tid, char* dst, i32 size);

bool Texture_Save(Crate* crate, TextureId tid, Guid* dst);
bool Texture_Load(Crate* crate, Guid name, TextureId* dst);

bool Texture_Unpalette(
    u8 const *const pim_noalias bytes,
    int2 size,
    const char* name,
    Material const *const material,
    float4 flatRome,
    TextureId *const albedoOut,
    TextureId *const romeOut,
    TextureId *const normalOut);

PIM_C_END
