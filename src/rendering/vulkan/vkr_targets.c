#include "rendering/vulkan/vkr_targets.h"

#include "rendering/vulkan/vkr_image.h"
#include "rendering/r_constants.h"
#include <string.h>

// TODO: scaled render resolution
static i32 GetDesiredWidth(void)
{
    return vkrGetDisplayWidth();
    //return (i32)(vkrGetDisplayWidth() * vkrGetRenderScale() + 0.5f);
}
static i32 GetDesiredHeight(void)
{
    return vkrGetDisplayHeight();
    //return (i32)(vkrGetDisplayHeight() * vkrGetRenderScale() + 0.5f);
}

static vkrTargets ms_targets;

bool vkrTargets_Init(void)
{
    bool success = true;

    vkrTargets* t = &ms_targets;
    memset(t, 0, sizeof(*t));

    i32 width = GetDesiredWidth();
    i32 height = GetDesiredHeight();
    if (!width || !height)
    {
        success = false;
        goto onfail;
    }

    t->width = width;
    t->height = height;
    for (i32 i = 0; i < NELEM(t->scene); ++i)
    {
        const u32 usage =
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT |
            VK_IMAGE_USAGE_STORAGE_BIT |
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
            VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        const u32 queueFamilyIndices[] =
        {
            g_vkr.queues[vkrQueueId_Graphics].family,
        };
        const VkImageCreateInfo info =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = VK_FORMAT_R16G16B16A16_SFLOAT,
            .extent.width = width,
            .extent.height = height,
            .extent.depth = 1,
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = NELEM(queueFamilyIndices),
            .pQueueFamilyIndices = queueFamilyIndices,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };
        if (!vkrImage_New(&t->scene[i], &info, vkrMemUsage_GpuOnly))
        {
            success = false;
            goto onfail;
        }
    }

    for (i32 i = 0; i < NELEM(t->depth); ++i)
    {
        const u32 usage =
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT;
        const u32 queueFamilyIndices[] =
        {
            g_vkr.queues[vkrQueueId_Graphics].family,
        };
        const VkImageCreateInfo info =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = VK_FORMAT_X8_D24_UNORM_PACK32,
            .extent.width = width,
            .extent.height = height,
            .extent.depth = 1,
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = NELEM(queueFamilyIndices),
            .pQueueFamilyIndices = queueFamilyIndices,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };
        if (!vkrImage_New(&t->depth[i], &info, vkrMemUsage_GpuOnly))
        {
            success = false;
            goto onfail;
        }
    }

onfail:
    if (!success)
    {
        vkrTargets_Shutdown();
    }
    return success;
}

void vkrTargets_Shutdown(void)
{
    vkrTargets* t = &ms_targets;

    for (i32 i = 0; i < NELEM(t->scene); ++i)
    {
        vkrImage_Release(&t->scene[i]);
    }
    for (i32 i = 0; i < NELEM(t->depth); ++i)
    {
        vkrImage_Release(&t->depth[i]);
    }

    memset(t, 0, sizeof(*t));
}

void vkrTargets_Recreate(void)
{
    if (vkrGetRenderWidth() != GetDesiredWidth() ||
        vkrGetRenderHeight() != GetDesiredHeight())
    {
        vkrTargets_Shutdown();
        vkrTargets_Init();
    }
}

i32 vkrGetRenderWidth(void)
{
    return ms_targets.width;
}

i32 vkrGetRenderHeight(void)
{
    return ms_targets.height;
}

float vkrGetRenderAspect(void)
{
    return (float)ms_targets.width / (float)ms_targets.height;
}

float vkrGetRenderScale(void)
{
    return r_scale_get();
}

i32 vkrGetDisplayWidth(void)
{
    return vkrGetBackBuffer()->width;
}

i32 vkrGetDisplayHeight(void)
{
    return vkrGetBackBuffer()->height;
}

vkrImage* vkrGetBackBuffer(void)
{
    vkrImage* img = &g_vkr.chain.images[vkrGetSwapIndex()];
    ASSERT(img->handle);
    return img;
}

vkrImage* vkrGetDepthBuffer(void)
{
    vkrImage* img = &ms_targets.depth[vkrGetSyncIndex()];
    ASSERT(img->handle);
    return img;
}

vkrImage* vkrGetSceneBuffer(void)
{
    vkrImage* img = &ms_targets.scene[vkrGetSyncIndex()];
    ASSERT(img->handle);
    return img;
}

vkrImage* vkrGetPrevSceneBuffer(void)
{
    vkrImage* img = &ms_targets.scene[vkrGetPrevSyncIndex()];
    ASSERT(img->handle);
    return img;
}
