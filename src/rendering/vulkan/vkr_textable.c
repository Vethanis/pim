#include "rendering/vulkan/vkr_textable.h"
#include "rendering/vulkan/vkr_desc.h"
#include "rendering/vulkan/vkr_texture.h"
#include "rendering/texture.h"
#include "rendering/lightmap.h"
#include "containers/table.h"
#include "common/profiler.h"
#include <string.h>

bool vkrTexTable_New(vkrTexTable* table)
{
    ASSERT(table);
    memset(table, 0, sizeof(*table));
    const u32 black = 0x0;
    if (!vkrTexture2D_New(
        &table->black, 1, 1, VK_FORMAT_R8G8B8A8_SRGB, &black, sizeof(black)))
    {
        return false;
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

ProfileMark(pm_update, vkrTexTable_Update)
void vkrTexTable_Update(vkrTexTable* table)
{
    ProfileBegin(pm_update);

    const i32 width = texture_table()->width;
    const texture_t* textures = texture_table()->values;
    const vkrTexture2D nullTexture = table->black;
    const i32 bindingCount = NELEM(table->table);
    VkDescriptorImageInfo* pim_noalias bindings = table->table;
    ASSERT(width <= bindingCount);
    for (i32 i = 0; i < bindingCount; ++i)
    {
        const vkrTexture2D* texture = &nullTexture;
        if ((i < width) && textures[i].texels)
        {
            texture = &textures[i].vkrtex;
        }
        bindings[i].sampler = texture->sampler;
        bindings[i].imageView = texture->view;
        bindings[i].imageLayout = texture->layout;
    }
    {
        const lmpack_t* lmpack = lmpack_get();
        i32 iBinding = width;
        for (i32 ilm = 0; ilm < lmpack->lmCount; ++ilm)
        {
            const lightmap_t* lm = lmpack->lightmaps + ilm;
            for (i32 idir = 0; idir < kGiDirections; ++idir)
            {
                if (iBinding < kTextureDescriptors)
                {
                    const vkrTexture2D* texture = &lm->vkrtex[idir];
                    if (texture->layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
                    {
                        bindings[iBinding].sampler = texture->sampler;
                        bindings[iBinding].imageView = texture->view;
                        bindings[iBinding].imageLayout = texture->layout;
                    }
                }
                else
                {
                    ASSERT(false);
                }
                ++iBinding;
            }
        }
    }

    ProfileEnd(pm_update);
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
        NELEM(table->table),
        table->table);

    ProfileEnd(pm_write);
}
