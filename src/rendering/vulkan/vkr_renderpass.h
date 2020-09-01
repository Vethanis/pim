#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

VkRenderPass vkrRenderPass_New(
    i32 attachmentCount,
    const VkAttachmentDescription* pAttachments,
    i32 subpassCount,
    const VkSubpassDescription* pSubpasses,
    i32 dependencyCount,
    const VkSubpassDependency* pDependencies);

void vkrRenderPass_Del(VkRenderPass pass);

PIM_C_END
