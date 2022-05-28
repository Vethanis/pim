#include "rendering/vulkan/vkr_pass.h"

#include "rendering/vulkan/vkr_pipeline.h"
#include "rendering/vulkan/vkr_desc.h"
#include "rendering/vulkan/vkr_bindings.h"
#include "rendering/vulkan/vkr_cmd.h"

#include <string.h>

bool vkrPass_New(vkrPass *const pass, vkrPassDesc const *const desc)
{
    ASSERT(pass);
    ASSERT(desc);
    memset(pass, 0, sizeof(*pass));

    VkShaderStageFlags stageFlags = 0x0;
    for (i32 i = 0; i < desc->shaderCount; ++i)
    {
        stageFlags |= desc->shaders[i].stage;
    }

    VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    if (stageFlags & VK_SHADER_STAGE_COMPUTE_BIT)
    {
        bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
    }
    const VkShaderStageFlags kRTStages =
        VK_SHADER_STAGE_RAYGEN_BIT_KHR |
        VK_SHADER_STAGE_ANY_HIT_BIT_KHR |
        VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
        VK_SHADER_STAGE_MISS_BIT_KHR |
        VK_SHADER_STAGE_INTERSECTION_BIT_KHR |
        VK_SHADER_STAGE_CALLABLE_BIT_KHR;
    if (stageFlags & kRTStages)
    {
        bindPoint = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
    }

    const VkDescriptorSetLayout setLayouts[] =
    {
        vkrBindings_GetSetLayout(),
    };
    i32 rangeCount = desc->pushConstantBytes > 0 ? 1 : 0;
    const VkPushConstantRange ranges[] =
    {
        {
            .stageFlags = stageFlags,
            .size = desc->pushConstantBytes,
        },
    };

    pass->bindpoint = bindPoint;
    pass->stageFlags = stageFlags;
    pass->pushConstantBytes = desc->pushConstantBytes;
    ASSERT(desc->pushConstantBytes <= 256);

    pass->layout = vkrPipelineLayout_New(
        NELEM(setLayouts), setLayouts,
        rangeCount, ranges);
    if (!pass->layout)
    {
        vkrPass_Del(pass);
        return false;
    }

    switch (bindPoint)
    {
    default:
        ASSERT(false);
        break;
    case VK_PIPELINE_BIND_POINT_COMPUTE:
    {
        ASSERT(desc->shaderCount == 1);
        pass->pipeline = vkrPipeline_NewComp(pass->layout, &desc->shaders[0]);
    }
    break;
    case VK_PIPELINE_BIND_POINT_GRAPHICS:
    {
        ASSERT(desc->shaderCount == 2);
        pass->pipeline = vkrPipeline_NewGfx(
            &desc->fixedFuncs,
            &desc->vertLayout,
            pass->layout,
            desc->renderPass,
            desc->subpass,
            desc->shaderCount,
            desc->shaders);
    }
    break;
    }

    if (!pass->pipeline)
    {
        vkrPass_Del(pass);
        return false;
    }

    return true;
}

void vkrPass_Del(vkrPass *const pass)
{
    if (pass)
    {
        vkrPipelineLayout_Del(pass->layout);
        vkrPipeline_Del(pass->pipeline);
        memset(pass, 0, sizeof(*pass));
    }
}
