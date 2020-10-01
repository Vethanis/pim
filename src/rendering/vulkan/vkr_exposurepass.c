#include "rendering/vulkan/vkr_exposurepass.h"
#include "rendering/vulkan/vkr_renderpass.h"
#include "rendering/vulkan/vkr_pipeline.h"
#include "rendering/vulkan/vkr_compile.h"
#include "rendering/vulkan/vkr_shader.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_desc.h"
#include "rendering/vulkan/vkr_buffer.h"
#include "rendering/vulkan/vkr_mem.h"
#include "rendering/vulkan/vkr_context.h"
#include "rendering/vulkan/vkr_sync.h"
#include "rendering/vulkan/vkr_pass.h"

#include "rendering/exposure.h"

#include "allocator/allocator.h"
#include "common/profiler.h"
#include "common/time.h"
#include "math/scalar.h"
#include <string.h>

bool vkrExposurePass_New(vkrExposurePass* pass)
{
	ASSERT(pass);
	memset(pass, 0, sizeof(*pass));
	bool success = true;

	VkPipelineShaderStageCreateInfo buildShaders[1] = { 0 };
	VkPipelineShaderStageCreateInfo adaptShaders[1] = { 0 };
	if (!vkrShader_New(&buildShaders[0], "BuildHistogram.hlsl", "CSMain", vkrShaderType_Comp))
	{
		success = false;
		goto cleanup;
	}
	if (!vkrShader_New(&adaptShaders[0], "AdaptHistogram.hlsl", "CSMain", vkrShaderType_Comp))
	{
		success = false;
		goto cleanup;
	}
	for (i32 i = 0; i < kFramesInFlight; ++i)
	{
		const i32 bytes = sizeof(u32) * kHistogramSize;
		if (!vkrBuffer_New(
			&pass->histBuffers[i],
			bytes,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			vkrMemUsage_CpuToGpu,
			PIM_FILELINE))
		{
			success = false;
			goto cleanup;
		}
	}
	for (i32 i = 0; i < kFramesInFlight; ++i)
	{
		const i32 bytes = sizeof(float) * 2;
		if (!vkrBuffer_New(
			&pass->expBuffers[i],
			bytes,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			vkrMemUsage_CpuToGpu,
			PIM_FILELINE))
		{
			success = false;
			goto cleanup;
		}
	}
	const VkDescriptorSetLayoutBinding bindings[] =
	{
		{
			// input luminance storage image. must be in layout VK_IMAGE_LAYOUT_GENERAL
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		},
		{
			// histogram storage buffer
			.binding = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		},
		{
			// exposure storage buffer
			.binding = 3,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		},
	};
	const VkPushConstantRange ranges[] =
	{
		{
			.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
			.size = sizeof(vkrExposureConstants),
		},
	};
	const VkDescriptorPoolSize poolSizes[] =
	{
		{
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 2,
		},
		{
			.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.descriptorCount = 1,
		},
	};

	const vkrPassDesc desc =
	{
		.bindpoint = VK_PIPELINE_BIND_POINT_COMPUTE,
		.poolSizeCount = NELEM(poolSizes),
		.poolSizes = poolSizes,
		.bindingCount = NELEM(bindings),
		.bindings = bindings,
		.rangeCount = NELEM(ranges),
		.ranges = ranges,
		.shaderCount = NELEM(buildShaders),
		.shaders = buildShaders,
	};
	if (!vkrPass_New(&pass->pass, &desc))
	{
		success = false;
		goto cleanup;
	}
	pass->adapt = vkrPipeline_NewComp(pass->pass.layout, &adaptShaders[0]);
	if (!pass->adapt)
	{
		success = false;
		goto cleanup;
	}

cleanup:
	for (i32 i = 0; i < NELEM(buildShaders); ++i)
	{
		vkrShader_Del(&buildShaders[i]);
	}
	for (i32 i = 0; i < NELEM(adaptShaders); ++i)
	{
		vkrShader_Del(&adaptShaders[i]);
	}
	if (!success)
	{
		vkrExposurePass_Del(pass);
	}
	return success;
}

void vkrExposurePass_Del(vkrExposurePass* pass)
{
	if (pass)
	{
		vkrPipeline_Del(pass->adapt);
		vkrPass_Del(&pass->pass);
		for (i32 i = 0; i < NELEM(pass->histBuffers); ++i)
		{
			vkrBuffer_Del(&pass->histBuffers[i]);
		}
		for (i32 i = 0; i < NELEM(pass->expBuffers); ++i)
		{
			vkrBuffer_Del(&pass->expBuffers[i]);
		}
		memset(pass, 0, sizeof(*pass));
	}
}

ProfileMark(pm_execute, vkrExposurePass_Execute)
void vkrExposurePass_Execute(vkrExposurePass* pass)
{
	ProfileBegin(pm_execute);

	float dt = (float)time_dtf();
	float mean = pass->params.deltaTime;
	mean = f1_lerp(mean, dt, 1.0f / 666.0f);
	pass->params.deltaTime = mean;

	const vkrSwapchain* chain = &g_vkr.chain;
	const u32 imgIndex = chain->imageIndex;
	const u32 syncIndex = chain->syncIndex;
	const vkrAttachment* lum = &chain->lumAttachments[imgIndex];
	VkBuffer histBuffer = pass->histBuffers[syncIndex].handle;
	VkBuffer expBuffer = pass->expBuffers[syncIndex].handle;
	VkDescriptorSet set = pass->pass.sets[syncIndex];
	VkPipelineLayout layout = pass->pass.layout;
	VkPipeline buildPipe = pass->pass.pipeline;
	VkPipeline adaptPipe = pass->adapt;

	const u32 gfxFamily = g_vkr.queues[vkrQueueId_Gfx].family;
	const u32 cmpFamily = g_vkr.queues[vkrQueueId_Comp].family;

	vkrThreadContext* ctx = vkrContext_Get();

	const VkImageMemoryBarrier lumToCompute =
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.newLayout = VK_IMAGE_LAYOUT_GENERAL,
		.srcQueueFamilyIndex = gfxFamily,
		.dstQueueFamilyIndex = cmpFamily,
		.image = lum->image.handle,
		.subresourceRange =
		{
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1,
		},
	};
	const VkImageMemoryBarrier lumToGraphics =
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_GENERAL,
		.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.srcQueueFamilyIndex = cmpFamily,
		.dstQueueFamilyIndex = gfxFamily,
		.image = lum->image.handle,
		.subresourceRange =
		{
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1,
		},
	};

	const VkBufferMemoryBarrier expToCompute =
	{
		.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
		.srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
		.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
		.srcQueueFamilyIndex = gfxFamily,
		.dstQueueFamilyIndex = cmpFamily,
		.buffer = expBuffer,
		.size = VK_WHOLE_SIZE,
	};
	const VkBufferMemoryBarrier buildToAdapt =
	{
		.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
		.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.buffer = histBuffer,
		.size = VK_WHOLE_SIZE,
	};
	const VkBufferMemoryBarrier expToGraphics =
	{
		.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
		.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
		.srcQueueFamilyIndex = cmpFamily,
		.dstQueueFamilyIndex = gfxFamily,
		.buffer = expBuffer,
		.size = VK_WHOLE_SIZE,
	};

	// update lum binding
	{
		const vkrBinding bindings[] =
		{
			{
				.set = set,
				.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				.binding = 0,
				.image =
				{
					.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
					.imageView = lum->view,
				},
			},
			{
				.set = set,
				.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.binding = 1,
				.buffer =
				{
					.buffer = histBuffer,
					.range = VK_WHOLE_SIZE,
				},
			},
			{
				.set = set,
				.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.binding = 3,
				.buffer =
				{
					.buffer = expBuffer,
					.range = VK_WHOLE_SIZE,
				},
			},
		};
		vkrDesc_WriteBindings(NELEM(bindings), bindings);
	}

	// transition lum img and exposure buf to compute
	{
		VkFence gfxfence = NULL;
		VkQueue gfxqueue = NULL;
		VkCommandBuffer gfxcmd = vkrContext_GetTmpCmd(ctx, vkrQueueId_Gfx, &gfxfence, &gfxqueue);
		vkrCmdBegin(gfxcmd);

		vkrCmdImageBarrier(
			gfxcmd,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			&lumToCompute);

		vkrCmdBufferBarrier(
			gfxcmd,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			&expToCompute);

		vkrCmdEnd(gfxcmd);
		vkrCmdSubmit(gfxqueue, gfxcmd, gfxfence, NULL, 0x0, NULL);
	}

	// dispatch shaders
	{
		VkFence cmpfence = NULL;
		VkQueue cmpqueue = NULL;
		VkCommandBuffer cmpcmd = vkrContext_GetTmpCmd(ctx, vkrQueueId_Comp, &cmpfence, &cmpqueue);
		vkrCmdBegin(cmpcmd);

		vkCmdBindDescriptorSets(
			cmpcmd, VK_PIPELINE_BIND_POINT_COMPUTE, layout, 0, 1, &set, 0, NULL);

		const i32 width = chain->width;
		const i32 height = chain->height;
		const vkrExposureConstants constants =
		{
			.width = width,
			.height = height,
			.exposure = pass->params,
		};
		vkCmdPushConstants(
			cmpcmd,
			layout,
			VK_SHADER_STAGE_COMPUTE_BIT,
			0,
			sizeof(vkrExposureConstants),
			&constants);

		// transition lum and exposure to compute
		{
			vkrCmdImageBarrier(
				cmpcmd,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				&lumToCompute);
			vkrCmdBufferBarrier(
				cmpcmd,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				&expToCompute);
		}

		// build histogram
		{
			const i32 dispatchX = (width + 15) / 16;
			const i32 dispatchY = (height + 15) / 16;
			vkCmdBindPipeline(cmpcmd, VK_PIPELINE_BIND_POINT_COMPUTE, buildPipe);
			vkCmdDispatch(cmpcmd, dispatchX, dispatchY, 1);
		}

		// barrier between build and adapt shaders
		{
			vkrCmdBufferBarrier(
				cmpcmd,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				&buildToAdapt);
		}

		// adapt exposure
		{
			vkCmdBindPipeline(cmpcmd, VK_PIPELINE_BIND_POINT_COMPUTE, adaptPipe);
			vkCmdDispatch(cmpcmd, 1, 1, 1);
		}

		// return lum and exposure to gfx
		{
			vkrCmdImageBarrier(
				cmpcmd,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				&lumToGraphics);
			vkrCmdBufferBarrier(
				cmpcmd,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				&expToGraphics);
		}

		vkrCmdEnd(cmpcmd);
		vkrCmdSubmit(cmpqueue, cmpcmd, cmpfence, NULL, 0x0, NULL);
	}

	// transition to gfx
	{
		VkFence gfxfence = NULL;
		VkQueue gfxqueue = NULL;
		VkCommandBuffer gfxcmd = vkrContext_GetTmpCmd(ctx, vkrQueueId_Gfx, &gfxfence, &gfxqueue);
		vkrCmdBegin(gfxcmd);

		vkrCmdImageBarrier(
			gfxcmd,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			&lumToGraphics);

		vkrCmdBufferBarrier(
			gfxcmd,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			&expToGraphics);

		vkrCmdEnd(gfxcmd);
		vkrCmdSubmit(gfxqueue, gfxcmd, gfxfence, NULL, 0x0, NULL);
	}

	ProfileEnd(pm_execute);
}
