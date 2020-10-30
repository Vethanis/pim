#include "rendering/vulkan/vkr_attachment.h"
#include "rendering/vulkan/vkr_image.h"
#include "rendering/vulkan/vkr_mem.h"
#include "rendering/vulkan/vkr_context.h"
#include "rendering/vulkan/vkr_cmd.h"
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
    if (!vkrImage_New(&att->image, &info, vkrMemUsage_GpuOnly, PIM_FILELINE))
    {
        ASSERT(false);
        success = false;
        goto cleanup;
    }
    att->view = vkrImageView_New(
        att->image.handle,
        VK_IMAGE_VIEW_TYPE_2D,
        format,
        aspect,
        0, 1,
        0, 1);
    if (!att->view)
    {
        ASSERT(false);
        success = false;
        goto cleanup;
    }

cleanup:
    if (!success)
    {
        vkrAttachment_Del(att);
    }
    return success;
}

void vkrAttachment_Del(vkrAttachment* att)
{
    if (att)
    {
        vkrImageView_Del(att->view);
        vkrImage_Del(&att->image);
        memset(att, 0, sizeof(*att));
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
