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
