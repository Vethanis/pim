#include "rendering/vulkan/vkr_pass.h"

#include "rendering/vulkan/vkr_pipeline.h"
#include "rendering/vulkan/vkr_desc.h"

#include <string.h>

bool vkrPass_New(vkrPass* pass, const vkrPassDesc* desc)
{
    ASSERT(pass);
    ASSERT(desc);
    memset(pass, 0, sizeof(*pass));

    pass->setLayout = vkrSetLayout_New(desc->bindingCount, desc->bindings, 0x0);
    if (!pass->setLayout)
    {
        vkrPass_Del(pass);
        return false;
    }

    pass->layout = vkrPipelineLayout_New(1, &pass->setLayout, desc->rangeCount, desc->ranges);
    if (!pass->layout)
    {
        vkrPass_Del(pass);
        return false;
    }

    if (desc->poolSizeCount > 0)
	{
		pass->descPool = vkrDescPool_New(kFramesInFlight, desc->poolSizeCount, desc->poolSizes);
		if (!pass->descPool)
		{
			vkrPass_Del(pass);
			return false;
		}

		for (i32 i = 0; i < kFramesInFlight; ++i)
		{
			pass->sets[i] = vkrDesc_New(pass->descPool, pass->setLayout);
			if (!pass->sets[i])
			{
				vkrPass_Del(pass);
				return false;
			}
		}
    }

    switch (desc->bindpoint)
    {
    default: ASSERT(false);
        break;
    case VK_PIPELINE_BIND_POINT_COMPUTE:
    {
        ASSERT(desc->shaderCount == 1);
        pass->pipeline = vkrPipeline_NewComp(pass->layout, &desc->shaders[0]);
    }
    break;
    case VK_PIPELINE_BIND_POINT_GRAPHICS:
    {
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

void vkrPass_Del(vkrPass* pass)
{
    if (pass)
    {
        vkrSetLayout_Del(pass->setLayout);
        vkrPipelineLayout_Del(pass->layout);
        vkrDescPool_Del(pass->descPool);
        vkrPipeline_Del(pass->pipeline);
        memset(pass, 0, sizeof(*pass));
    }
}
