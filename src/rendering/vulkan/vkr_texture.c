#include "rendering/vulkan/vkr_texture.h"
#include "rendering/vulkan/vkr_mem.h"
#include "rendering/vulkan/vkr_context.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "allocator/allocator.h"
#include "common/time.h"
#include "math/scalar.h"
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

    const i32 mipCount = 1 + (i32)floorf(log2f((float)i1_max(width, height)));
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
        .mipLevels = mipCount,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage =
            VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = NELEM(queueFamilies),
        .pQueueFamilyIndices = queueFamilies,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    tex->format = format;
    if (!vkrImage_New(
        &tex->image,
        &imageInfo,
        vkrMemUsage_GpuOnly,
        PIM_FILELINE))
    {
        success = false;
        goto cleanup;
    }

    VkImage image = tex->image.handle;
    ASSERT(image);

    // create image view
    const VkImageViewCreateInfo viewInfo =
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.levelCount = mipCount,
        .subresourceRange.layerCount = 1,
    };
    VkCheck(vkCreateImageView(g_vkr.dev, &viewInfo, g_vkr.alloccb, &tex->view));
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
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .anisotropyEnable = true,
        .maxAnisotropy = 8.0f,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .minLod = 0.0f,
        .maxLod = (float)mipCount,
        .mipLodBias = 0.0f,
    };
    VkCheck(vkCreateSampler(g_vkr.dev, &samplerInfo, g_vkr.alloccb, &tex->sampler));
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
        const VkBufferMemoryBarrier stageBarrier =
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask = 0x0,
            .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer = stagebuf.handle,
            .offset = 0,
            .size = VK_WHOLE_SIZE,
        };
        vkrCmdBufferBarrier(
            cmd,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            &stageBarrier);

        // transition all mips from undefined to xfer dst
        VkImageMemoryBarrier barrier =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = 0x0,
            .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = image,
            .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = mipCount,
                .layerCount = 1,
            },
        };
        vkrCmdImageBarrier(
            cmd,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            &barrier);

        // copy buffer to image mip 0
        const VkBufferImageCopy region =
        {
            .imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .imageSubresource.mipLevel = 0,
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

        barrier.subresourceRange.levelCount = 1;
        // generate mips
        for (i32 i = 1; i < mipCount; ++i)
        {
            // transition (i-1) to xfer src optimal
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            vkrCmdImageBarrier(
                cmd,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                &barrier);

            // blit (i-1) into i
            i32 srcWidth = i1_max(width >> (i - 1), 1);
            i32 srcHeight = i1_max(height >> (i - 1), 1);
            i32 dstWidth = i1_max(width >> i, 1);
            i32 dstHeight = i1_max(height >> i, 1);
            const VkImageBlit blit =
            {
                .srcOffsets[1] = { srcWidth, srcHeight, 1 },
                .dstOffsets[1] = { dstWidth, dstHeight, 1 },
                .srcSubresource =
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = i - 1,
                    .layerCount = 1,
                },
                .dstSubresource =
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = i,
                    .layerCount = 1,
                },
            };
            vkCmdBlitImage(
                cmd,
                image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &blit,
                VK_FILTER_LINEAR);

            // transition (i-1) to shader read
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            vkrCmdImageBarrier(
                cmd,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                &barrier);
        }

        // transition last mip to shader read
        barrier.subresourceRange.baseMipLevel = mipCount - 1;
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
        if (tex->image.handle)
        {
            // create pipeline barrier to safely release resources
            const VkImageMemoryBarrier barrier =
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask = 0x0,
                .dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
                .oldLayout = tex->layout,
                .newLayout = VK_IMAGE_LAYOUT_GENERAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = tex->image.handle,
                .subresourceRange =
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .levelCount = 1,
                    .layerCount = 1,
                },
            };
            VkFence fence = vkrMem_Barrier(
                vkrQueueId_Gfx,
                VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
                VK_PIPELINE_STAGE_HOST_BIT,
                NULL,
                NULL,
                &barrier);

            if (tex->sampler)
            {
                const vkrReleasable releasable =
                {
                    .frame = time_framecount(),
                    .type = vkrReleasableType_Sampler,
                    .fence = fence,
                    .sampler = tex->sampler,
                };
                vkrReleasable_Add(&g_vkr.allocator, &releasable);
            }
            if (tex->view)
            {
                const vkrReleasable releasable =
                {
                    .frame = time_framecount(),
                    .type = vkrReleasableType_ImageView,
                    .fence = fence,
                    .view = tex->view,
                };
                vkrReleasable_Add(&g_vkr.allocator, &releasable);
            }
            vkrImage_Release(&tex->image, fence);
        }
        memset(tex, 0, sizeof(*tex));
    }
}
