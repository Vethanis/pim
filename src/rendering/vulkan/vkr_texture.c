#include "rendering/vulkan/vkr_texture.h"
#include "rendering/vulkan/vkr_image.h"
#include "rendering/vulkan/vkr_context.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_buffer.h"
#include "rendering/vulkan/vkr_mem.h"
#include "allocator/allocator.h"
#include "common/time.h"
#include "math/scalar.h"
#include <string.h>

i32 vkrTexture2D_MipCount(i32 width, i32 height)
{
    return 1 + (i32)floorf(log2f((float)i1_max(width, height)));
}

bool vkrTexture2D_New(
    vkrTexture2D* tex,
    i32 width,
    i32 height,
    VkFormat format,
    const void* data,
    i32 bytes)
{
    bool success = true;
    ASSERT(tex);
    memset(tex, 0, sizeof(*tex));

    if (bytes < 0)
    {
        ASSERT(false);
        return false;
    }

    tex->width = width;
    tex->height = height;
    tex->format = format;
    tex->layout = VK_IMAGE_LAYOUT_UNDEFINED;

    const i32 mipCount = vkrTexture2D_MipCount(width, height);
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

    VkImageView view = vkrImageView_New(
        image,
        VK_IMAGE_VIEW_TYPE_2D,
        format,
        VK_IMAGE_ASPECT_COLOR_BIT,
        0, mipCount,
        0, 1);
    tex->view = view;
    ASSERT(view);
    if (!view)
    {
        success = false;
        goto cleanup;
    }

    VkSampler sampler = vkrSampler_New(
        VK_FILTER_LINEAR,
        VK_SAMPLER_MIPMAP_MODE_LINEAR,
        VK_SAMPLER_ADDRESS_MODE_REPEAT, 
        8.0f);
    tex->sampler = sampler;
    ASSERT(sampler);
    if (!sampler)
    {
        success = false;
        goto cleanup;
    }

    if (bytes > 0)
    {
        vkrTexture2D_Upload(tex, data, bytes);
    }

cleanup:
    if (!success)
    {
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
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_GENERAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = tex->image.handle,
                .subresourceRange =
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .levelCount = VK_REMAINING_MIP_LEVELS,
                    .layerCount = VK_REMAINING_ARRAY_LAYERS,
                },
            };
            VkFence fence = vkrMem_Barrier(
                vkrQueueId_Gfx,
                VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                VK_PIPELINE_STAGE_HOST_BIT,
                NULL,
                NULL,
                &barrier);

            vkrSampler_Release(tex->sampler, fence);
            vkrImageView_Release(tex->view, fence);
            vkrImage_Release(&tex->image, fence);
        }
        memset(tex, 0, sizeof(*tex));
    }
}

VkFence vkrTexture2D_Upload(vkrTexture2D* tex, const void* data, i32 bytes)
{
    ASSERT(tex);
    ASSERT(tex->image.handle);
    ASSERT(bytes >= 0);
    ASSERT(data || !bytes);
    if (bytes <= 0)
    {
        return NULL;
    }

    vkrBuffer stagebuf = { 0 };
    if (!vkrBuffer_New(
        &stagebuf,
        bytes,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        vkrMemUsage_CpuOnly,
        PIM_FILELINE))
    {
        return NULL;
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

    const i32 width = tex->width;
    const i32 height = tex->height;
    const i32 mipCount = vkrTexture2D_MipCount(width, height);
    VkImage image = tex->image.handle;

    vkrThreadContext* ctx = vkrContext_Get();
    VkFence fence = NULL;
    VkQueue queue = NULL;
    VkCommandBuffer cmd = vkrContext_GetTmpCmd(ctx, vkrQueueId_Gfx, &fence, &queue);
    vkrCmdBegin(cmd);
    {
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
                .levelCount = VK_REMAINING_MIP_LEVELS,
                .layerCount = VK_REMAINING_ARRAY_LAYERS,
            },
        };
        vkrCmdImageBarrier(
            cmd,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            &barrier);
        tex->layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

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
            image,
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

    return fence;
}
