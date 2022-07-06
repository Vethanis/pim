#include "rendering/vulkan/vkr_image.h"

#include "rendering/vulkan/vkr_mem.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_context.h"
#include "rendering/vulkan/vkr_queue.h"

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
    if (!handle)
    {
        success = false;
        goto cleanup;
    }
    image->handle = handle;
    image->allocation = NULL;
    image->view = NULL;
    image->type = info->imageType;
    image->format = info->format;
    image->state.layout = info->initialLayout;
    image->usage = info->usage;
    image->width = info->extent.width;
    image->height = info->extent.height;
    image->depth = info->extent.depth;
    image->mipLevels = info->mipLevels;
    image->arrayLayers = info->arrayLayers;
    image->imported = 1;

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
        image->handle = NULL;
        vkrMem_ImageDel(image);
    }
    return success;
}

void vkrImage_Release(vkrImage* image)
{
    ASSERT(image);
    const vkrSubmitId submitId = vkrImage_GetSubmit(image);
    const u32 attachUsage =
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
    if (image->usage & attachUsage)
    {
        vkrAttachment_Release(image->view, submitId);
        image->view = NULL;
    }
    if (image->imported)
    {
        ASSERT(!image->allocation);
        vkrImageView_Release(image->view, submitId);
        image->view = NULL;
        image->handle = NULL;
        image->allocation = NULL;
    }
    else if (image->handle || image->allocation || image->view)
    {
        const vkrReleasable releasable =
        {
            .submitId = submitId,
            .type = vkrReleasableType_Image,
            .image.handle = image->handle,
            .image.allocation = image->allocation,
            .image.view = image->view,
        };
        vkrReleasable_Add(&releasable);
        image->handle = NULL;
        image->allocation = NULL;
        image->view = NULL;
    }
    memset(image, 0, sizeof(*image));
}

void vkrImageView_Release(VkImageView view, vkrSubmitId submitId)
{
    if (view)
    {
        const vkrReleasable releasable =
        {
            .submitId = submitId,
            .type = vkrReleasableType_ImageView,
            .view = view,
        };
        vkrReleasable_Add(&releasable);
    }
}

void vkrAttachment_Release(VkImageView view, vkrSubmitId submitId)
{
    if (view)
    {
        const vkrReleasable releasable =
        {
            .submitId = submitId,
            .type = vkrReleasableType_Attachment,
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
    u32 i = vkrGetSyncIndex();
    ASSERT(i < NELEM(set->frames));
    return &set->frames[i];
}

vkrImage* vkrImageSet_Prev(vkrImageSet* set)
{
    u32 i = vkrGetPrevSyncIndex();
    ASSERT(i < NELEM(set->frames));
    return &set->frames[i];
}

vkrImage* vkrImageSet_Next(vkrImageSet* set)
{
    u32 i = vkrGetNextSyncIndex();
    ASSERT(i < NELEM(set->frames));
    return &set->frames[i];
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
