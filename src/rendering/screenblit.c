#include "screenblit.h"
#include "rendering/vulkan/vkr_buffer.h"
#include "rendering/vulkan/vkr_mem.h"
#include "rendering/vulkan/vkr_image.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_swapchain.h"
#include "rendering/constants.h"
#include "math/types.h"
#include "allocator/allocator.h"
#include "common/profiler.h"
#include <string.h>

static const float2 kScreenMesh[] =
{
    { -1.0f, -1.0f },
    { 1.0f, -1.0f },
    { 1.0f,  1.0f },
    { 1.0f,  1.0f },
    { -1.0f,  1.0f },
    { -1.0f, -1.0f },
};

// ----------------------------------------------------------------------------

bool vkrScreenBlit_New(vkrScreenBlit* blit)
{
    ASSERT(blit);
    memset(blit, 0, sizeof(*blit));
    bool success = true;

    const u32 queueFamilies[] =
    {
        g_vkr.queues[vkrQueueId_Gfx].family,
    };
    const VkImageCreateInfo imageInfo =
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags = 0x0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_SRGB,
        .extent.width = kDrawWidth,
        .extent.height = kDrawHeight,
        .extent.depth = 1,
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage =
            VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = NELEM(queueFamilies),
        .pQueueFamilyIndices = queueFamilies,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    if (!vkrImage_New(
        &blit->image,
        &imageInfo,
        vkrMemUsage_GpuOnly,
        PIM_FILELINE))
    {
        ASSERT(false);
        success = false;
        goto cleanup;
    }
    if (!vkrBuffer_New(
        &blit->meshbuf,
        sizeof(kScreenMesh),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        vkrMemUsage_CpuToGpu,
        PIM_FILELINE))
    {
        ASSERT(false);
        success = false;
        goto cleanup;
    }
    if (!vkrBuffer_New(
        &blit->stagebuf,
        kDrawWidth * kDrawHeight * 4,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        vkrMemUsage_CpuOnly,
        PIM_FILELINE))
    {
        ASSERT(false);
        success = false;
        goto cleanup;
    }

cleanup:
    if (!success)
    {
        vkrScreenBlit_Del(blit);
    }
    return success;
}

void vkrScreenBlit_Del(vkrScreenBlit* blit)
{
    if (blit)
    {
        vkrImage_Del(&blit->image);
        vkrBuffer_Del(&blit->meshbuf);
        vkrBuffer_Del(&blit->stagebuf);
        memset(blit, 0, sizeof(*blit));
    }
}

ProfileMark(pm_blit, vkrScreenBlit_Blit)
void vkrScreenBlit_Blit(
    vkrScreenBlit* blit,
    VkCommandBuffer cmd,
    VkImage dstImage,
    u32 const *const pim_noalias texels,
    i32 width,
    i32 height)
{
    ProfileBegin(pm_blit);

    vkrSwapchain const *const chain = &g_vkr.chain;
    VkImage srcImage = blit->image.handle;
    VkBuffer stageBuf = blit->stagebuf.handle;

    // copy input data to stage buffer
    {
        vkrBuffer* buffer = &blit->stagebuf;
        const i32 bytes = width * height * sizeof(texels[0]);
        ASSERT(bytes == buffer->size);
        void* const pim_noalias dst = vkrBuffer_Map(buffer);
        ASSERT(dst);
        if (dst)
        {
            memcpy(dst, texels, bytes);
        }
        vkrBuffer_Unmap(buffer);
        vkrBuffer_Flush(buffer);
    }

    // transition src image to xfer dst
    {
        const VkImageMemoryBarrier barrier =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = 0x0,
            .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = srcImage,
            .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1,
            },
        };
        vkrCmdImageBarrier(cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            &barrier);
    }
    // copy buffer to src image
    {
        const VkBufferImageCopy bufferRegion =
        {
            .imageSubresource =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .layerCount = 1,
            },
            .imageExtent.width = width,
            .imageExtent.height = height,
            .imageExtent.depth = 1,
        };
        vkCmdCopyBufferToImage(
            cmd,
            stageBuf,
            srcImage,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &bufferRegion);
    }
    // transition src image to xfer src
    {
        const VkImageMemoryBarrier barrier =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = 0x0,
            .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = srcImage,
            .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1,
            },
        };
        vkrCmdImageBarrier(cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            &barrier);
    }
    // transition dst image to xfer dst
    {
        const VkImageMemoryBarrier barrier =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = 0x0,
            .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = dstImage,
            .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1,
            },
        };
        vkrCmdImageBarrier(cmd,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            &barrier);
    }
    // blit src image to dst image
    {
        const VkImageBlit region =
        {
            .srcOffsets[0] = { 0, height, 0 },
            .srcOffsets[1] = { width, 0, 1 },
            .dstOffsets[1] = { chain->width, chain->height, 1 },
            .srcSubresource =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .layerCount = 1,
            },
            .dstSubresource =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .layerCount = 1,
            },
        };
        vkCmdBlitImage(
            cmd,
            srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &region,
            VK_FILTER_LINEAR);
    }
    // transition dst image to attachment
    {
        const VkImageMemoryBarrier barrier =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = 0x0,
            .dstAccessMask = 0x0,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = dstImage,
            .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1,
            },
        };
        vkrCmdImageBarrier(cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            &barrier);
    }

    ProfileEnd(pm_blit);
}
