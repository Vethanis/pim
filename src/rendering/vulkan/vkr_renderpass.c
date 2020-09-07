#include "rendering/vulkan/vkr_renderpass.h"
#include "allocator/allocator.h"
#include <string.h>

VkRenderPass vkrRenderPass_New(
    i32 attachmentCount,
    const VkAttachmentDescription* pAttachments,
    i32 subpassCount,
    const VkSubpassDescription* pSubpasses,
    i32 dependencyCount,
    const VkSubpassDependency* pDependencies)
{
    const VkRenderPassCreateInfo createInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = attachmentCount,
        .pAttachments = pAttachments,
        .subpassCount = subpassCount,
        .pSubpasses = pSubpasses,
        .dependencyCount = dependencyCount,
        .pDependencies = pDependencies,
    };
    VkRenderPass handle = NULL;
    VkCheck(vkCreateRenderPass(g_vkr.dev, &createInfo, g_vkr.alloccb, &handle));
    ASSERT(handle);
    return handle;
}

void vkrRenderPass_Del(VkRenderPass pass)
{
    if (pass)
    {
        vkDestroyRenderPass(g_vkr.dev, pass, g_vkr.alloccb);
    }
}
