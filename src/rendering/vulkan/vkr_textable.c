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

bool vkrTexTable_New(vkrTexTable* table)
{
    ASSERT(table);
    memset(table, 0, sizeof(*table));
    const u32 blackTexel = 0x0;
    if (!vkrTexture2D_New(
        &table->black,
        1, 1,
        VK_FORMAT_R8G8B8A8_SRGB,
        &blackTexel, sizeof(blackTexel)))
    {
        return false;
    }
    const vkrTexture2D* nullTex = &table->black;
    VkDescriptorImageInfo* infos = table->table;
    for (i32 i = 0; i < kTextureDescriptors; ++i)
    {
        infos[i].imageLayout = nullTex->image.layout;
        infos[i].imageView = nullTex->view;
        infos[i].sampler = nullTex->sampler;
    }
    return true;
}

void vkrTexTable_Del(vkrTexTable* table)
{
    if (table)
    {
        vkrTexture2D_Del(&table->black);
        memset(table, 0, sizeof(*table));
    }
}

void vkrTexTable_Update(vkrTexTable* table)
{
    const table_t* cpuTable = texture_table();
    const texture_t* textures = cpuTable->values;
    vkrTexture2D const *const black = &table->black;
    VkDescriptorImageInfo *const descriptors = table->table;
    const i32 minWidth = i1_min(cpuTable->width, kTextureDescriptors);
    i32 i = 0;
    for (; i < minWidth; ++i)
    {
        vkrTexture2D const* gpuTex = black;
        if (textures[i].vkrtex.image.handle)
        {
            gpuTex = &textures[i].vkrtex;
        }
        descriptors[i].imageLayout = gpuTex->image.layout;
        descriptors[i].imageView = gpuTex->view;
        descriptors[i].sampler = gpuTex->sampler;
    }
    for (; i < kTextureDescriptors; ++i)
    {
        descriptors[i].imageLayout = black->image.layout;
        descriptors[i].imageView = black->view;
        descriptors[i].sampler = black->sampler;
    }
}

ProfileMark(pm_write, vkrTexTable_Write)
void vkrTexTable_Write(const vkrTexTable* table, VkDescriptorSet set)
{
    ProfileBegin(pm_write);

    ASSERT(set);
    vkrDesc_WriteImageTable(
        set,
        2, // must match TextureTable.hlsl
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        kTextureDescriptors,
        table->table);

    ProfileEnd(pm_write);
}

void vkrTexTable_WriteSlot(
    vkrTexTable* table,
    i32 slot,
    VkSampler sampler,
    VkImageView view,
    VkImageLayout layout)
{
    ASSERT(slot >= 0);
    ASSERT(slot < kTextureDescriptors);
    ASSERT(sampler);
    ASSERT(view);
    ASSERT(layout);
    if (slot > 0)
    {
        table->table[slot].sampler = sampler;
        table->table[slot].imageView = view;
        table->table[slot].imageLayout = layout;
    }
}

void vkrTexTable_ClearSlot(vkrTexTable* table, i32 slot)
{
    ASSERT(slot >= 0);
    ASSERT(slot < kTextureDescriptors);
    if (slot >= 0)
    {
        const vkrTexture2D* black = &table->black;
        table->table[slot].imageLayout = black->image.layout;
        table->table[slot].imageView = black->view;
        table->table[slot].sampler = black->sampler;
    }
}
