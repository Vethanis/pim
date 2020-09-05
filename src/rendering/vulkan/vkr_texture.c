#include "rendering/vulkan/vkr_texture.h"
#include "rendering/vulkan/vkr_mem.h"
#include "rendering/vulkan/vkr_context.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "allocator/allocator.h"
#include <string.h>

bool vkrTexture2D_New(
    vkrTexture2D* tex,
    i32 width,
    i32 height,
    VkFormat format,
    const void* data,
    i32 bytes)
{
    bool success = true;
    vkrBuffer stagebuf = { 0 };
    ASSERT(tex);
    memset(tex, 0, sizeof(*tex));

    if (bytes < 0)
    {
        success = false;
        goto cleanup;
    }

    if (!vkrBuffer_New(
        &stagebuf,
        bytes,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        vkrMemUsage_CpuOnly,
        PIM_FILELINE))
    {
        success = false;
        goto cleanup;
    }

    {
        void* dst = vkrBuffer_Map(&stagebuf);
        ASSERT(dst);
        if (dst)
        {
            memcpy(dst, data, bytes);
        }
        vkrBuffer_Unmap(&stagebuf);
        vkrBuffer_Flush(&stagebuf);
    }

    const u32 queueFamilies[] =
    {
        g_vkr.queues[vkrQueueId_Gfx].family,
    };
    const VkImageCreateInfo imageInfo =
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags = 0x0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent.width = width,
        .extent.height = height,
        .extent.depth = 1,
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = NELEM(queueFamilies),
        .pQueueFamilyIndices = queueFamilies,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    if (!vkrImage_New(
        &tex->image,
        &imageInfo,
        vkrMemUsage_GpuOnly,
        PIM_FILELINE))
    {
        success = false;
        goto cleanup;
    }

    // create image view
    const VkImageViewCreateInfo viewInfo =
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = tex->image.handle,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.levelCount = 1,
        .subresourceRange.layerCount = 1,
    };
    VkCheck(vkCreateImageView(g_vkr.dev, &viewInfo, NULL, &tex->view));
    ASSERT(tex->view);
    if (!tex->view)
    {
        success = false;
        goto cleanup;
    }

    // create image sampler
    const VkSamplerCreateInfo samplerInfo =
    {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .flags = 0x0,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .mipLodBias = 0.0f,
        .anisotropyEnable = true,
        .maxAnisotropy = 8.0f,
        .minLod = 0.0f,
        .maxLod = 0.0f,
    };
    VkCheck(vkCreateSampler(g_vkr.dev, &samplerInfo, NULL, &tex->sampler));
    ASSERT(tex->sampler);
    if (!tex->sampler)
    {
        success = false;
        goto cleanup;
    }

    vkrFrameContext* ctx = vkrContext_Get();
    VkCommandBuffer cmd = NULL;
    VkFence fence = NULL;
    VkQueue queue = NULL;
    vkrContext_GetCmd(ctx, vkrQueueId_Gfx, &cmd, &fence, &queue);
    vkrCmdBegin(cmd);
    {
        // transition from undefined to xfer dst
        VkImageMemoryBarrier barrier =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = 0x0,
            .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = tex->image.handle,
            .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .layerCount = 1,
                .levelCount = 1,
            },
        };
        vkrCmdImageBarrier(
            cmd,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            &barrier);

        // copy buffer to image
        const VkBufferImageCopy region =
        {
            .imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .imageSubresource.layerCount = 1,
            .imageExtent.width = width,
            .imageExtent.height = height,
            .imageExtent.depth = 1,
        };
        vkCmdCopyBufferToImage(
            cmd,
            stagebuf.handle,
            tex->image.handle,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &region);

        // transition from xfer dst to shader read
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        vkrCmdImageBarrier(
            cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            &barrier);
    }
    vkrCmdEnd(cmd);
    vkrCmdSubmit(queue, cmd, fence, NULL, 0x0, NULL);
    vkrBuffer_Release(&stagebuf, fence);

    tex->layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    tex->width = width;
    tex->height = height;

cleanup:
    if (!success)
    {
        vkrBuffer_Del(&stagebuf);
        vkrTexture2D_Del(tex);
    }
    return success;
}

void vkrTexture2D_Del(vkrTexture2D* tex)
{
    if (tex)
    {
        if (tex->sampler)
        {
            vkDestroySampler(g_vkr.dev, tex->sampler, NULL);
        }
        if (tex->view)
        {
            vkDestroyImageView(g_vkr.dev, tex->view, NULL);
        }
        vkrImage_Release(&tex->image, NULL);
        memset(tex, 0, sizeof(*tex));
    }
}
