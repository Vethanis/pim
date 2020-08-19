#include "rendering/vulkan/vkr_renderpass.h"
#include "allocator/allocator.h"
#include <string.h>

vkrRenderPass* vkrRenderPass_New(
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
    VkCheck(vkCreateRenderPass(g_vkr.dev, &createInfo, NULL, &handle));
    if (handle)
    {
        vkrRenderPass* pass = perm_calloc(sizeof(*pass));
        pass->refcount = 1;
        pass->handle = handle;
        pass->attachmentCount = attachmentCount;
        pass->subpassCount = subpassCount;
        return pass;
    }
    return NULL;
}

void vkrRenderPass_Retain(vkrRenderPass* pass)
{
    if (pass && pass->refcount > 0)
    {
        pass->refcount += 1;
    }
}

void vkrRenderPass_Release(vkrRenderPass* pass)
{
    if (pass && pass->refcount > 0)
    {
        pass->refcount -= 1;
        if (pass->refcount == 0)
        {
            if (pass->handle)
            {
                vkDestroyRenderPass(g_vkr.dev, pass->handle, NULL);
            }
            memset(pass, 0, sizeof(*pass));
            pim_free(pass);
        }
    }
}
