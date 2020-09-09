#include "rendering/vulkan/vkr_image.h"
#include "rendering/vulkan/vkr_mem.h"
#include "VulkanMemoryAllocator/src/vk_mem_alloc.h"
#include "allocator/allocator.h"
#include "common/profiler.h"
#include "common/time.h"
#include <string.h>

ProfileMark(pm_imgnew, vkrImage_New)
bool vkrImage_New(
    vkrImage* image,
    const VkImageCreateInfo* info,
    vkrMemUsage memUsage,
    const char* tag)
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
            .frame = time_framecount(),
            .type = vkrReleasableType_Image,
            .fence = fence,
            .image = *image,
        };
        vkrReleasable_Add(&g_vkr.allocator, &releasable);
    }
    memset(image, 0, sizeof(*image));
    ProfileEnd(pm_imgrelease);
}

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
