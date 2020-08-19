#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

vkrRenderPass* vkrRenderPass_New(
    i32 attachmentCount,
    const VkAttachmentDescription* pAttachments,
    i32 subpassCount,
    const VkSubpassDescription* pSubpasses,
    i32 dependencyCount,
    const VkSubpassDependency* pDependencies);

void vkrRenderPass_Retain(vkrRenderPass* pass);
void vkrRenderPass_Release(vkrRenderPass* pass);

PIM_C_END
