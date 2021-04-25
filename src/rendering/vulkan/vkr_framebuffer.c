#include "rendering/vulkan/vkr_framebuffer.h"
#include "rendering/vulkan/vkr_renderpass.h"

#include "allocator/allocator.h"
#include <string.h>

bool vkrFramebuffer_New(
    vkrFramebuffer* fbuf,
    const VkImageView* attachments,
    const VkFormat* formats,
    i32 attachmentCount,
    i32 width,
    i32 height)
{
    ASSERT(attachments);
    ASSERT(formats);
    ASSERT(width > 0);
    ASSERT(height > 0);
    ASSERT(attachmentCount > 0);
    ASSERT(attachmentCount <= 8);
    attachmentCount = pim_min(attachmentCount, 8);

    memset(fbuf, 0, sizeof(*fbuf));
    memcpy(fbuf->attachments, attachments, sizeof(attachments[0]) * attachmentCount);
    memcpy(fbuf->formats, formats, sizeof(formats[0]) * attachmentCount);
    fbuf->width = width;
    fbuf->height = height;

    // make up a compatible render pass
    vkrRenderPassDesc passDesc =
    {
        .srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    };
    for (i32 i = 0; i < attachmentCount; ++i)
    {
        passDesc.attachments[i].format = formats[i];
        VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        if ((i == 0) &&
            (formats[i] >= VK_FORMAT_D16_UNORM) &&
            (formats[i] <= VK_FORMAT_D32_SFLOAT_S8_UINT))
        {
            layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }
        passDesc.attachments[i].initialLayout = layout;
        passDesc.attachments[i].layout = layout;
        passDesc.attachments[i].finalLayout = layout;
        passDesc.attachments[i].load = VK_ATTACHMENT_LOAD_OP_CLEAR;
        passDesc.attachments[i].load = VK_ATTACHMENT_STORE_OP_STORE;
    }
    VkRenderPass pass = vkrRenderPass_Get(&passDesc);
    ASSERT(pass);
    if (pass)
    {
        const VkFramebufferCreateInfo bufferInfo =
        {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = pass,
            .attachmentCount = attachmentCount,
            .pAttachments = attachments,
            .width = width,
            .height = height,
            .layers = 1,
        };
        VkCheck(vkCreateFramebuffer(g_vkr.dev, &bufferInfo, NULL, &fbuf->handle));
        ASSERT(fbuf->handle);
    }

    return fbuf->handle != NULL;
}

void vkrFramebuffer_Del(vkrFramebuffer* fbuf)
{
    if (fbuf->handle)
    {
        ASSERT(g_vkr.dev);
        vkDestroyFramebuffer(g_vkr.dev, fbuf->handle, NULL);
    }
    memset(fbuf, 0, sizeof(*fbuf));
}
