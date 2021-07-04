#include "rendering/vulkan/vkr_image.h"

#include "rendering/vulkan/vkr_mem.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_context.h"

#include "allocator/allocator.h"
#include "common/profiler.h"
#include "common/time.h"
#include "common/console.h"

#include <string.h>

// ----------------------------------------------------------------------------

bool vkrImage_New(
    vkrImage* image,
    const VkImageCreateInfo* info,
    vkrMemUsage memUsage)
{
    return vkrMem_ImageNew(image, info, memUsage);
}

bool vkrImage_Import(
    vkrImage* image,
    const VkImageCreateInfo* info,
    VkImage handle)
{
    ASSERT(info);
    ASSERT(handle);

    bool success = true;
    memset(image, 0, sizeof(*image));

    image->handle = handle;
    image->allocation = NULL;
    image->view = NULL;
    image->type = info->imageType;
    image->format = info->format;
    image->layout = info->initialLayout;
    image->usage = info->usage;
    image->width = info->extent.width;
    image->height = info->extent.height;
    image->depth = info->extent.depth;
    image->mipLevels = info->mipLevels;
    image->arrayLayers = info->arrayLayers;
    image->imported = 1;

    if (!handle)
    {
        success = false;
        goto cleanup;
    }

    VkImageViewCreateInfo viewInfo;
    if (vkrImage_InfoToViewInfo(info, &viewInfo))
    {
        viewInfo.image = image->handle;
        VkCheck(vkCreateImageView(g_vkr.dev, &viewInfo, NULL, &image->view));
        if (!image->view)
        {
            success = false;
            goto cleanup;
        }
    }

cleanup:
    if (!success)
    {
        vkrMem_ImageDel(image);
    }
    return success;
}

void vkrImage_Release(vkrImage* image)
{
    ASSERT(image);
    if (image->imported)
    {
        ASSERT(!image->allocation);
        vkrImageView_Release(image->view);
    }
    else
    {
        if (image->handle || image->allocation || image->view)
        {
            const vkrReleasable releasable =
            {
                .frame = vkrSys_FrameIndex(),
                .type = vkrReleasableType_Image,
                .image.handle = image->handle,
                .image.allocation = image->allocation,
                .image.view = image->view,
            };
            vkrReleasable_Add(&releasable);
        }
    }
    memset(image, 0, sizeof(*image));
}

void vkrImageView_Release(VkImageView view)
{
    if (view)
    {
        const vkrReleasable releasable =
        {
            .type = vkrReleasableType_ImageView,
            .frame = Time_FrameCount(),
            .view = view,
        };
        vkrReleasable_Add(&releasable);
    }
}

bool vkrImage_Reserve(
    vkrImage* image,
    const VkImageCreateInfo* info,
    vkrMemUsage memUsage)
{
    ASSERT(!image->imported);
    bool success = true;
    if ((image->type != info->imageType) ||
        (image->format != info->format) ||
        (image->width != info->extent.width) ||
        (image->height != info->extent.height) ||
        (image->depth != info->extent.depth) ||
        (image->mipLevels != info->mipLevels) ||
        (image->arrayLayers != info->arrayLayers) ||
        (image->usage != info->usage))
    {
        vkrImage_Release(image);
        success = vkrImage_New(image, info, memUsage);
    }
    return success;
}

void* vkrImage_Map(const vkrImage* image)
{
    ASSERT(image);
    return vkrMem_Map(image->allocation);
}

void vkrImage_Unmap(const vkrImage* image)
{
    ASSERT(image);
    vkrMem_Unmap(image->allocation);
}

void vkrImage_Flush(const vkrImage* image)
{
    ASSERT(image);
    vkrMem_Flush(image->allocation);
}

void vkrImage_Barrier(
    vkrImage* image,
    VkCommandBuffer cmd,
    VkImageLayout newLayout,
    VkAccessFlags srcAccessMask,
    VkAccessFlags dstAccessMask,
    VkPipelineStageFlags srcStages,
    VkPipelineStageFlags dstStages)
{
    VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    if (image->usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
    {
        aspect = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    const VkImageMemoryBarrier barrier =
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = srcAccessMask,
        .dstAccessMask = dstAccessMask,
        .oldLayout = image->layout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image->handle,
        .subresourceRange =
        {
            .aspectMask = aspect,
            .levelCount = image->mipLevels,
            .layerCount = image->arrayLayers,
        },
    };
    vkrCmdImageBarrier(
        cmd,
        srcStages,
        dstStages,
        &barrier);
    image->layout = newLayout;
}

void vkrImage_Transfer(
    vkrImage* image,
    vkrQueueId srcQueueId, vkrQueueId dstQueueId,
    VkCommandBuffer srcCmd, VkCommandBuffer dstCmd,
    VkImageLayout newLayout,
    VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
    VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask)
{
    VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    if (image->usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
    {
        aspect = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    u32 srcQueueFamily = g_vkr.queues[srcQueueId].family;
    u32 dstQueueFamily = g_vkr.queues[dstQueueId].family;
    VkImageLayout oldLayout = image->layout;
    bool wasUndefined = oldLayout == VK_IMAGE_LAYOUT_UNDEFINED;
    const VkImageMemoryBarrier barrier =
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = wasUndefined ? 0x0 : srcAccessMask,
        .dstAccessMask = dstAccessMask,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = srcQueueFamily,
        .dstQueueFamilyIndex = dstQueueFamily,
        .image = image->handle,
        .subresourceRange =
        {
            .aspectMask = aspect,
            .levelCount = image->mipLevels,
            .layerCount = image->arrayLayers,
        },
    };
    vkrCmdImageBarrier(
        srcCmd,
        srcStageMask,
        dstStageMask,
        &barrier);
    if (srcQueueFamily != dstQueueFamily)
    {
        vkrCmdImageBarrier(
            dstCmd,
            srcStageMask,
            dstStageMask,
            &barrier);
    }
    image->layout = newLayout;
}

// ----------------------------------------------------------------------------

bool vkrImageSet_New(
    vkrImageSet* set,
    const VkImageCreateInfo* info,
    vkrMemUsage memUsage)
{
    memset(set, 0, sizeof(*set));
    for (i32 i = 0; i < NELEM(set->frames); ++i)
    {
        if (!vkrImage_New(&set->frames[i], info, memUsage))
        {
            vkrImageSet_Release(set);
            return false;
        }
    }
    return true;
}

void vkrImageSet_Release(vkrImageSet* set)
{
    for (i32 i = 0; i < NELEM(set->frames); ++i)
    {
        vkrImage_Release(&set->frames[i]);
    }
    memset(set, 0, sizeof(*set));
}

vkrImage* vkrImageSet_Current(vkrImageSet* set)
{
    u32 syncIndex = vkrSys_SyncIndex();
    ASSERT(syncIndex < NELEM(set->frames));
    return &set->frames[syncIndex];
}

vkrImage* vkrImageSet_Prev(vkrImageSet* set)
{
    u32 prevIndex = (vkrSys_SyncIndex() + (kResourceSets - 1u)) % kResourceSets;
    ASSERT(prevIndex < NELEM(set->frames));
    return &set->frames[prevIndex];
}

bool vkrImageSet_Reserve(
    vkrImageSet* set,
    const VkImageCreateInfo* info,
    vkrMemUsage memUsage)
{
    return vkrImage_Reserve(vkrImageSet_Current(set), info, memUsage);
}

// ----------------------------------------------------------------------------

VkImageViewType vkrImage_InfoToViewType(const VkImageCreateInfo* info)
{
    VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D;
    switch (info->imageType)
    {
    default:
    case VK_IMAGE_TYPE_2D:
        if (info->arrayLayers <= 1)
        {
            viewType = VK_IMAGE_VIEW_TYPE_2D;
        }
        else if (info->arrayLayers == 6)
        {
            // may also want 2darray view for uav cubemap use
            viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        }
        else
        {
            viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        }
        break;
    case VK_IMAGE_TYPE_1D:
        viewType = (info->arrayLayers <= 1) ?
            VK_IMAGE_VIEW_TYPE_1D : VK_IMAGE_VIEW_TYPE_1D_ARRAY;
        break;
    case VK_IMAGE_TYPE_3D:
        viewType = VK_IMAGE_VIEW_TYPE_3D;
        break;
    }
    return viewType;
}

VkImageAspectFlags vkrImage_InfoToAspects(const VkImageCreateInfo* info)
{
    VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    switch (info->format)
    {
        // default to color aspect
    default:
        aspect = VK_IMAGE_ASPECT_COLOR_BIT;
        break;
        // depth only formats:
    case VK_FORMAT_D16_UNORM:
    case VK_FORMAT_X8_D24_UNORM_PACK32:
    case VK_FORMAT_D32_SFLOAT:
        aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
        break;
        // depth and stencil formats:
    case VK_FORMAT_D16_UNORM_S8_UINT:
    case VK_FORMAT_D24_UNORM_S8_UINT:
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        aspect = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        break;
        // stencil only formats:
    case VK_FORMAT_S8_UINT:
        aspect = VK_IMAGE_ASPECT_STENCIL_BIT;
        break;
    }
    return aspect;
}

bool vkrImage_InfoToViewInfo(const VkImageCreateInfo* info, VkImageViewCreateInfo* viewInfo)
{
    memset(viewInfo, 0, sizeof(*viewInfo));
    // anything but transfer usage needs a view
    const VkImageUsageFlags viewless =
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
        VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (info->usage & (~viewless))
    {
        viewInfo->sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo->format = info->format;
        viewInfo->viewType = vkrImage_InfoToViewType(info);
        viewInfo->subresourceRange.aspectMask = vkrImage_InfoToAspects(info);
        viewInfo->subresourceRange.baseMipLevel = 0;
        viewInfo->subresourceRange.levelCount = info->mipLevels;
        viewInfo->subresourceRange.baseArrayLayer = 0;
        viewInfo->subresourceRange.layerCount = info->arrayLayers;
        return true;
    }
    return false;
}
