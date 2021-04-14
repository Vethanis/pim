#include "rendering/vulkan/vkr_attachment.h"
#include "rendering/vulkan/vkr_image.h"
#include "rendering/vulkan/vkr_mem.h"
#include "rendering/vulkan/vkr_context.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "allocator/allocator.h"
#include <string.h>

bool vkrAttachment_New(
    vkrImage* att,
    i32 width,
    i32 height,
    VkFormat format,
    VkImageUsageFlags usage)
{
    bool success = true;
    ASSERT(att);
    ASSERT(width > 0);
    ASSERT(height > 0);
    memset(att, 0, sizeof(*att));

    VkImageAspectFlags aspect = 0x0;
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
    {
        aspect = VK_IMAGE_ASPECT_COLOR_BIT;
        layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    else if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
    {
        aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
        layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }
    ASSERT(aspect);

    const VkImageCreateInfo info = 
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent.width = width,
        .extent.height = height,
        .extent.depth = 1,
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = usage,
    };
    if (!vkrImage_New(att, &info, vkrMemUsage_GpuOnly))
    {
        ASSERT(false);
        success = false;
        goto cleanup;
    }

cleanup:
    if (!success)
    {
        vkrAttachment_Release(att);
    }
    return success;
}

void vkrAttachment_Release(vkrImage* att)
{
    vkrImage_Release(att);
}
