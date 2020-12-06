#include "rendering/vulkan/vkr_image.h"
#include "rendering/vulkan/vkr_mem.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_context.h"
#include "VulkanMemoryAllocator/src/vk_mem_alloc.h"
#include "allocator/allocator.h"
#include "common/profiler.h"
#include "common/time.h"
#include <string.h>

// ----------------------------------------------------------------------------

ProfileMark(pm_imgnew, vkrImage_New)
bool vkrImage_New(
    vkrImage* image,
    const VkImageCreateInfo* info,
    vkrMemUsage memUsage)
{
    ProfileBegin(pm_imgnew);
    memset(image, 0, sizeof(*image));
    const VmaAllocationCreateInfo allocInfo =
    {
        .flags = VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT,
        .usage = memUsage,
    };
    VkImage handle = NULL;
    VmaAllocation allocation = NULL;
    VkCheck(vmaCreateImage(
        g_vkr.allocator.handle,
        info,
        &allocInfo,
        &handle,
        &allocation,
        NULL));
    ProfileEnd(pm_imgnew);
    if (handle)
    {
        image->handle = handle;
        image->allocation = allocation;
        image->type = info->imageType;
        image->format = info->format;
        image->layout = info->initialLayout;
        image->usage = info->usage;
        image->width = info->extent.width;
        image->height = info->extent.height;
        image->depth = info->extent.depth;
        image->mipLevels = info->mipLevels;
        image->arrayLayers = info->arrayLayers;
        return true;
    }
    return false;
}

ProfileMark(pm_imgdel, vkrImage_Del)
void vkrImage_Del(vkrImage* image)
{
    ProfileBegin(pm_imgdel);
    if (image)
    {
        if (image->handle)
        {
            vmaDestroyImage(
                g_vkr.allocator.handle,
                image->handle,
                image->allocation);
        }
        memset(image, 0, sizeof(*image));
    }
    ProfileEnd(pm_imgdel);
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

ProfileMark(pm_imgrelease, vkrImage_Release)
void vkrImage_Release(vkrImage* image, VkFence fence)
{
    ProfileBegin(pm_imgrelease);
    ASSERT(image);
    if (image->handle)
    {
        const vkrReleasable releasable =
        {
            .frame = vkr_frameIndex(),
            .type = vkrReleasableType_Image,
            .fence = fence,
            .image = *image,
        };
        vkrReleasable_Add(&g_vkr.allocator, &releasable);
    }
    memset(image, 0, sizeof(*image));
    ProfileEnd(pm_imgrelease);
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

VkImageView vkrImageView_New(
    VkImage image,
    VkImageViewType type,
    VkFormat format,
    VkImageAspectFlags aspect,
    i32 baseMip, i32 mipCount,
    i32 baseLayer, i32 layerCount)
{
    ASSERT(image);
    const VkImageViewCreateInfo viewInfo =
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = type,
        .format = format,
        .subresourceRange =
        {
            .aspectMask = aspect,
            .baseMipLevel = baseMip,
            .levelCount = mipCount,
            .baseArrayLayer = baseLayer,
            .layerCount = layerCount,
        },
    };
    VkImageView handle = NULL;
    VkCheck(vkCreateImageView(g_vkr.dev, &viewInfo, NULL, &handle));
    ASSERT(handle);
    return handle;
}

void vkrImageView_Del(VkImageView view)
{
    if (view)
    {
        vkDestroyImageView(g_vkr.dev, view, NULL);
    }
}

void vkrImageView_Release(VkImageView view, VkFence fence)
{
    if (view)
    {
        const vkrReleasable releasable =
        {
            .type = vkrReleasableType_ImageView,
            .frame = time_framecount(),
            .fence = fence,
            .view = view,
        };
        vkrReleasable_Add(&g_vkr.allocator, &releasable);
    }
}

// ----------------------------------------------------------------------------

VkSampler vkrSampler_New(
    VkFilter filter,
    VkSamplerMipmapMode mipMode,
    VkSamplerAddressMode addressMode,
    float aniso)
{
    const VkSamplerCreateInfo info = 
    {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = filter,
        .minFilter = filter,
        .mipmapMode = mipMode,
        .addressModeU = addressMode,
        .addressModeV = addressMode,
        .addressModeW = addressMode,
        .mipLodBias = 0.0f,
        .anisotropyEnable = aniso > 1.0f,
        .maxAnisotropy = aniso,
        .minLod = 0.0f,
        .maxLod = VK_LOD_CLAMP_NONE,
    };
    VkSampler handle = NULL;
    VkCheck(vkCreateSampler(g_vkr.dev, &info, NULL, &handle));
    ASSERT(handle);
    return handle;
}

void vkrSampler_Del(VkSampler sampler)
{
    if (sampler)
    {
        vkDestroySampler(g_vkr.dev, sampler, NULL);
    }
}

void vkrSampler_Release(VkSampler sampler, VkFence fence)
{
    if (sampler)
    {
        const vkrReleasable releasable =
        {
            .type = vkrReleasableType_Sampler,
            .frame = time_framecount(),
            .fence = fence,
            .sampler = sampler,
        };
        vkrReleasable_Add(&g_vkr.allocator, &releasable);
    }
}

// ----------------------------------------------------------------------------
