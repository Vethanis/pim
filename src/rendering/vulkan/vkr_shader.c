#include "rendering/vulkan/vkr_shader.h"
#include "allocator/allocator.h"
#include <string.h>

static VkShaderModule vkrCreateShaderModule(const u32* dwords, i32 dwordCount)
{
    ASSERT(g_vkr.dev);
    VkShaderModule mod = NULL;
    if (dwords && dwordCount > 0)
    {
        const VkShaderModuleCreateInfo info =
        {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pCode = dwords,
            .codeSize = sizeof(dwords[0]) * dwordCount,
        };
        VkCheck(vkCreateShaderModule(g_vkr.dev, &info, g_vkr.alloccb, &mod));
    }
    return mod;
}

static void vkrDestroyShaderModule(VkShaderModule mod)
{
    ASSERT(g_vkr.dev);
    if (mod)
    {
        vkDestroyShaderModule(g_vkr.dev, mod, g_vkr.alloccb);
    }
}

i32 vkrShaderTypeToStage(vkrShaderType type)
{
    switch (type)
    {
    default:
        ASSERT(false);
        return 0;
    case vkrShaderType_Vert:
        return VK_SHADER_STAGE_VERTEX_BIT;
    case vkrShaderType_Frag:
        return VK_SHADER_STAGE_FRAGMENT_BIT;
    case vkrShaderType_Comp:
        return VK_SHADER_STAGE_COMPUTE_BIT;
    case vkrShaderType_Raygen:
        return VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_RAYGEN_BIT_NV;
    case vkrShaderType_AnyHit:
        return VK_SHADER_STAGE_ANY_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_NV;
    case vkrShaderType_ClosestHit:
        return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;
    case vkrShaderType_Miss:
        return VK_SHADER_STAGE_MISS_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_NV;
    case vkrShaderType_Isect:
        return VK_SHADER_STAGE_INTERSECTION_BIT_KHR | VK_SHADER_STAGE_INTERSECTION_BIT_NV;
    case vkrShaderType_Call:
        return VK_SHADER_STAGE_CALLABLE_BIT_KHR | VK_SHADER_STAGE_CALLABLE_BIT_NV;
    case vkrShaderType_Task:
        return VK_SHADER_STAGE_TASK_BIT_NV;
    case vkrShaderType_Mesh:
        return VK_SHADER_STAGE_MESH_BIT_NV;
    }
}

VkPipelineShaderStageCreateInfo vkrCreateShader(const vkrCompileOutput* output)
{
    VkPipelineShaderStageCreateInfo info =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = vkrShaderTypeToStage(output->type),
        .module = vkrCreateShaderModule(output->dwords, output->dwordCount),
        .pName = output->entrypoint,
    };
    return info;
}

void vkrDestroyShader(VkPipelineShaderStageCreateInfo* info)
{
    if (info)
    {
        vkrDestroyShaderModule(info->module);
        memset(info, 0, sizeof(*info));
    }
}
