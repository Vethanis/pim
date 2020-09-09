#include "rendering/vulkan/vkr_attachment.h"
#include "rendering/vulkan/vkr_image.h"
#include "rendering/vulkan/vkr_mem.h"
#include "allocator/allocator.h"
#include <string.h>

bool vkrAttachment_New(
    vkrAttachment* att,
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

    att->format = format;
    att->aspect = aspect;
    att->layout = VK_IMAGE_LAYOUT_UNDEFINED;

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
        .usage = usage | VK_IMAGE_USAGE_SAMPLED_BIT,
    };
    if (!vkrImage_New(&att->image, &info, vkrMemUsage_GpuOnly, PIM_FILELINE))
    {
        success = false;
        goto cleanup;
    }
    if (!vkrImageView_New(
        att->image.handle,
        VK_IMAGE_VIEW_TYPE_2D,
        format,
        aspect,
        0, 1,
        0, 1))
    {
        success = false;
        goto cleanup;
    }

cleanup:
    if (!success)
    {
        vkrAttachment_Del(att, NULL);
    }
    return success;
}

void vkrAttachment_Del(vkrAttachment* att, VkFence fence)
{
    if (att->image.handle)
    {
        // create pipeline barrier to safely release resources
        const VkImageMemoryBarrier barrier =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = 0x0,
            .dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_GENERAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = att->image.handle,
            .subresourceRange =
            {
                .aspectMask = att->aspect,
                .levelCount = VK_REMAINING_MIP_LEVELS,
                .layerCount = VK_REMAINING_ARRAY_LAYERS,
            },
        };
        VkFence fence = vkrMem_Barrier(
            vkrQueueId_Gfx,
            VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
            VK_PIPELINE_STAGE_HOST_BIT,
            NULL,
            NULL,
            &barrier);

        vkrImageView_Release(att->view, fence);
        vkrImage_Release(&att->image, fence);
    }
}

VkAttachmentDescription vkrAttachment_Desc(
    VkAttachmentLoadOp loadOp,
    VkAttachmentStoreOp storeOp,
    VkImageLayout initialLayout,
    VkImageLayout finalLayout,
    VkFormat format)
{
    const VkAttachmentDescription desc =
    {
        .flags = 0x0,
        .format = format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = loadOp,
        .storeOp = storeOp,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .initialLayout = initialLayout,
        .finalLayout = finalLayout,
    };
    return desc;
}
