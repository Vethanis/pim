#include "rendering/vulkan/vkr_textable.h"
#include "rendering/vulkan/vkr_desc.h"
#include "rendering/vulkan/vkr_texture.h"
#include "rendering/texture.h"
#include "rendering/lightmap.h"
#include "containers/table.h"
#include "common/profiler.h"
#include "common/find.h"
#include "allocator/allocator.h"
#include "math/scalar.h"
#include <string.h>

typedef struct vkrTexTable
{
    queue_t freelist;
    i32 length;
    u8 versions[kTextureDescriptors];
    vkrTexture2D textures[kTextureDescriptors];
    VkDescriptorImageInfo descriptors[kTextureDescriptors];
} vkrTexTable;

static vkrTexTable ms_table;

bool vkrTexTable_Init(void)
{
    vkrTexTable *const table = &ms_table;
    memset(table, 0, sizeof(*table));

    queue_create(&table->freelist, sizeof(u32), EAlloc_Perm);

    // slot 0 is always a 1x1 black texture
    table->length = 1;
    const u32 blackTexel = 0x0;
    if (!vkrTexture2D_New(
        &table->textures[0],
        1, 1,
        VK_FORMAT_R8G8B8A8_SRGB,
        &blackTexel, sizeof(blackTexel)))
    {
        return false;
    }

    vkrTexture2D const *const black = &table->textures[0];
    VkDescriptorImageInfo *const infos = table->descriptors;
    for (i32 i = 0; i < kTextureDescriptors; ++i)
    {
        infos[i].imageLayout = black->image.layout;
        infos[i].imageView = black->view;
        infos[i].sampler = black->sampler;
    }
    return true;
}

void vkrTexTable_Shutdown(void)
{
    vkrTexTable *const table = &ms_table;
    queue_destroy(&table->freelist);
    for (i32 i = 0; i < table->length; ++i)
    {
        vkrTexture2D_Del(&table->textures[i]);
    }
    memset(table, 0, sizeof(*table));
}

void vkrTexTable_Update(void)
{
    vkrTexture2D const *const textures = ms_table.textures;
    VkDescriptorImageInfo *const descriptors = ms_table.descriptors;
    for (i32 i = 0; i < kTextureDescriptors; ++i)
    {
        if (textures[i].view)
        {
            descriptors[i].imageLayout = textures[i].image.layout;
            descriptors[i].imageView = textures[i].view;
            descriptors[i].sampler = textures[i].sampler;
        }
        else
        {
            descriptors[i].imageLayout = textures[0].image.layout;
            descriptors[i].imageView = textures[0].view;
            descriptors[i].sampler = textures[0].sampler;
        }
    }
}

void vkrTexTable_Write(VkDescriptorSet set, i32 binding)
{
    ASSERT(set);
    vkrDesc_WriteImageTable(
        set,
        binding,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        kTextureDescriptors,
        ms_table.descriptors);
}

bool vkrTexTable_Exists(vkrTextureId id)
{
    i32 slot = id.index;
    return (slot > 0) && (ms_table.versions[slot] == id.version);
}

vkrTextureId vkrTexTable_Alloc(
    i32 width,
    i32 height,
    VkFormat format,
    void const *const data)
{
    vkrTexTable *const table = &ms_table;

    queue_t *const freelist = &table->freelist;
    u32 slot = 0;
    if (!queue_trypop(freelist, &slot, sizeof(slot)))
    {
        if (table->length < kTextureDescriptors)
        {
            slot = table->length++;
        }
    }
    if (slot <= 0)
    {
        ASSERT(false);
        return (vkrTextureId){ 0 };
    }

    vkrTexture2D *const tex = &table->textures[slot];
    i32 bpp = vkrFormatToBpp(format);
    i32 bytes = (width * height * bpp) / 8;
    if (!vkrTexture2D_New(tex, width, height, format, data, data ? bytes : 0))
    {
        queue_push(freelist, &slot, sizeof(slot));
        ASSERT(false);
        return (vkrTextureId) { 0 };
    }

    VkDescriptorImageInfo *const desc = &table->descriptors[slot];
    desc->imageLayout = tex->image.layout;
    desc->imageView = tex->view;
    desc->sampler = tex->sampler;

    vkrTextureId id = { 0 };
    id.index = slot;
    id.version = ++(table->versions[slot]);
    return id;
}

bool vkrTexTable_Free(vkrTextureId id)
{
    vkrTexTable *const table = &ms_table;

    i32 slot = id.index;
    if (vkrTexTable_Exists(id))
    {
        table->versions[slot]++;
        queue_push(&table->freelist, &slot, sizeof(slot));

        vkrTexture2D_Del(&table->textures[slot]);
        vkrTexture2D const *const black = &table->textures[0];
        table->descriptors[slot].imageLayout = black->image.layout;
        table->descriptors[slot].imageView = black->view;
        table->descriptors[slot].sampler = black->sampler;
        return true;
    }
    return false;
}

VkFence vkrTexTable_Upload(vkrTextureId id, void const *const data, i32 bytes)
{
    vkrTexTable *const table = &ms_table;

    if (vkrTexTable_Exists(id))
    {
        return vkrTexture2D_Upload(&table->textures[id.index], data, bytes);
    }
    return NULL;
}
