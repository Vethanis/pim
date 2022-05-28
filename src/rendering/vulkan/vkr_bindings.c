#include "rendering/vulkan/vkr_bindings.h"
#include "rendering/vulkan/vkr_desc.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_textable.h"
#include "rendering/vulkan/vkr_pipeline.h"

#include "common/profiler.h"
#include <string.h>

// ----------------------------------------------------------------------------

typedef struct vkrBinding
{
    VkDescriptorType type;
    union
    {
        VkDescriptorBufferInfo buffer;
        VkDescriptorImageInfo image;
    };
} vkrBinding;

// ----------------------------------------------------------------------------

static void vkrBindings_Flush(void);

// ----------------------------------------------------------------------------

static const VkDescriptorSetLayoutBinding kSetLayoutBindings[] =
{
    {
        .binding = bid_TextureTable1D,
        .descriptorType = bid_TextureTable1D_TYPE,
        .descriptorCount = bid_TextureTable1D_COUNT,
        .stageFlags = bid_TextureTable1D_STAGES,
    },
    {
        .binding = bid_TextureTable2D,
        .descriptorType = bid_TextureTable2D_TYPE,
        .descriptorCount = bid_TextureTable2D_COUNT,
        .stageFlags = bid_TextureTable2D_STAGES,
    },
    {
        .binding = bid_TextureTable3D,
        .descriptorType = bid_TextureTable3D_TYPE,
        .descriptorCount = bid_TextureTable3D_COUNT,
        .stageFlags = bid_TextureTable3D_STAGES,
    },
    {
        .binding = bid_TextureTableCube,
        .descriptorType = bid_TextureTableCube_TYPE,
        .descriptorCount = bid_TextureTableCube_COUNT,
        .stageFlags = bid_TextureTableCube_STAGES,
    },
    {
        .binding = bid_TextureTable2DArray,
        .descriptorType = bid_TextureTable2DArray_TYPE,
        .descriptorCount = bid_TextureTable2DArray_COUNT,
        .stageFlags = bid_TextureTable2DArray_STAGES,
    },
    {
        .binding = bid_Globals,
        .descriptorType = bid_Globals_TYPE,
        .descriptorCount = 1,
        .stageFlags = bid_Globals_STAGES,
    },
    {
        .binding = bid_SceneLuminance,
        .descriptorType = bid_SceneLuminance_TYPE,
        .descriptorCount = 1,
        .stageFlags = bid_SceneLuminance_STAGES,
    },
    {
        .binding = bid_RWSceneLuminance,
        .descriptorType = bid_RWSceneLuminance_TYPE,
        .descriptorCount = 1,
        .stageFlags = bid_RWSceneLuminance_STAGES,
    },
    {
        .binding = bid_HistogramBuffer,
        .descriptorType = bid_HistogramBuffer_TYPE,
        .descriptorCount = 1,
        .stageFlags = bid_HistogramBuffer_STAGES,
    },
    {
        .binding = bid_ExposureBuffer,
        .descriptorType = bid_ExposureBuffer_TYPE,
        .descriptorCount = 1,
        .stageFlags = bid_ExposureBuffer_STAGES,
    },
};
SASSERT(NELEM(kSetLayoutBindings) == bid_COUNT);

static const VkDescriptorPoolSize kPoolSizes[] =
{
    {
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = DescPool_UniformBuffer_COUNT,
    },
    {
        .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount = DescPool_StorageBuffer_COUNT,
    },
    {
        .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .descriptorCount = DescPool_StorageImage_COUNT,
    },
    {
        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = DescPool_CombinedImageSampler_COUNT,
    },
};

// ----------------------------------------------------------------------------

static VkDescriptorSetLayout ms_layout;
static VkDescriptorPool ms_pool;
static VkDescriptorSet ms_sets[R_ResourceSets];
static VkWriteDescriptorSet ms_writes[bid_COUNT];
static vkrBinding ms_bindings[bid_COUNT];
static i32 ms_dirty[bid_COUNT];

// ----------------------------------------------------------------------------

bool vkrBindings_Init(void)
{
    bool success = true;

    ms_layout = vkrDescSetLayout_New(NELEM(kSetLayoutBindings), kSetLayoutBindings, 0x0);
    if (!ms_layout)
    {
        success = false;
        goto onfail;
    }

    ms_pool = vkrDescPool_New(NELEM(ms_sets), NELEM(kPoolSizes), kPoolSizes);
    if (!ms_pool)
    {
        success = false;
        goto onfail;
    }

    for (i32 i = 0; i < NELEM(ms_sets); ++i)
    {
        ms_sets[i] = vkrDescSet_New(ms_pool, ms_layout);
        if (!ms_sets[i])
        {
            success = false;
            goto onfail;
        }
    }

onfail:
    if (!success)
    {
        vkrBindings_Shutdown();
    }

    return success;
}

ProfileMark(pm_update, vkrBindings_Update)
void vkrBindings_Update(void)
{
    ProfileBegin(pm_update);

    // why do validation layers think there is a command buffer in flight using this desc set?
    //vkrSubmit_AwaitAll();
    VkDescriptorSet set = vkrBindings_GetSet();
    if (set)
    {
        vkrTexTable_Write(set);
        vkrBindings_Flush();
    }

    ProfileEnd(pm_update);
}

void vkrBindings_Shutdown(void)
{
    if (ms_pool)
    {
        for (i32 i = 0; i < NELEM(ms_sets); ++i)
        {
            vkrDescSet_Del(ms_pool, ms_sets[i]);
            ms_sets[i] = NULL;
        }
        vkrDescPool_Del(ms_pool);
        ms_pool = NULL;
    }
    vkrDescSetLayout_Del(ms_layout);
    ms_layout = NULL;
}

VkDescriptorSetLayout vkrBindings_GetSetLayout(void)
{
    ASSERT(ms_layout);
    return ms_layout;
}

VkDescriptorSet vkrBindings_GetSet(void)
{
    u32 syncIndex = vkrGetSyncIndex();
    ASSERT(syncIndex < NELEM(ms_sets));
    ASSERT(ms_sets[syncIndex]);
    return ms_sets[syncIndex];
}

void vkrBindings_BindImage(
    i32 id,
    VkDescriptorType type,
    VkSampler sampler,
    VkImageView view,
    VkImageLayout layout)
{
    ASSERT((u32)id < (u32)NELEM(ms_bindings));
    ASSERT(view && sampler);
    ms_bindings[id].type = type;
    ms_bindings[id].image.imageLayout = layout;
    ms_bindings[id].image.imageView = view;
    ms_bindings[id].image.sampler = sampler;
    ms_dirty[id] = NELEM(ms_sets);
}

void vkrBindings_BindBuffer(
    i32 id,
    VkDescriptorType type,
    vkrBuffer const *const buffer)
{
    ASSERT((u32)id < (u32)NELEM(ms_bindings));
    ASSERT(buffer->handle);
    ms_bindings[id].type = type;
    ms_bindings[id].buffer.buffer = buffer->handle;
    ms_bindings[id].buffer.offset = 0;
    ms_bindings[id].buffer.range = buffer->size;
    ms_dirty[id] = NELEM(ms_sets);
}

ProfileMark(pm_flush, vkrBindings_Flush)
static void vkrBindings_Flush(void)
{
    ProfileBegin(pm_flush);

    VkDescriptorSet set = vkrBindings_GetSet();
    if (set)
    {
        i32 writeCount = 0;
        for (i32 i = 0; i < NELEM(ms_bindings); ++i)
        {
            ASSERT(ms_dirty[i] >= 0);
            if (!ms_dirty[i])
            {
                continue;
            }
            ms_dirty[i]--;

            VkWriteDescriptorSet *const write = &ms_writes[writeCount++];
            memset(write, 0, sizeof(*write));
            write->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write->descriptorType = ms_bindings[i].type;
            write->dstSet = set;
            write->dstBinding = i;
            write->dstArrayElement = 0;
            write->descriptorCount = 1;

            switch (ms_bindings[i].type)
            {
            default:
                ASSERT(false);
                break;
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
            {
                write->pBufferInfo = &ms_bindings[i].buffer;
            }
            break;
            case VK_DESCRIPTOR_TYPE_SAMPLER:
            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
            {
                write->pImageInfo = &ms_bindings[i].image;
            }
            break;
            }
        }
        if (writeCount > 0)
        {
            vkUpdateDescriptorSets(g_vkr.dev, writeCount, ms_writes, 0, NULL);
        }
    }

    ProfileEnd(pm_flush);
}
