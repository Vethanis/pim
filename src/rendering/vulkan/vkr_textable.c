#include "rendering/vulkan/vkr_textable.h"
#include "rendering/vulkan/vkr_desc.h"
#include "rendering/vulkan/vkr_texture.h"
#include "rendering/texture.h"
#include "rendering/lightmap.h"
#include "containers/table.h"
#include "common/profiler.h"
#include "common/find.h"
#include "allocator/allocator.h"
#include <string.h>

bool vkrTexTable_New(vkrTexTable* table)
{
    ASSERT(table);
    memset(table, 0, sizeof(*table));
    table->width = 1;
    const u32 blackTexel = 0x0;
    if (!vkrTexture2D_New(
        &table->black, 1, 1, VK_FORMAT_R8G8B8A8_SRGB, &blackTexel, sizeof(blackTexel)))
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

i32 vkrTexTable_AllocSlot(vkrTexTable* table)
{
    ASSERT(table);

    i32 slot = 0;
    if (table->width < kTextureDescriptors)
    {
        slot = table->width++;
    }
    else
    {
        ASSERT(false);
    }
    return slot;
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

        VkSampler blackSampler = black->sampler;
        i32 width = table->width;
        for (i32 i = width - 1; i > 0; --i)
        {
            if (table->table[i].sampler == blackSampler)
            {
                --width;
            }
            else
            {
                break;
            }
        }
        table->width = width;
    }
}
