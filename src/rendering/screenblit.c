#include "screenblit.h"
#include "rendering/vulkan/vkr_device.h"
#include "rendering/vulkan/vkr_buffer.h"
#include "rendering/vulkan/vkr_mem.h"
#include "rendering/vulkan/vkr_image.h"
#include "rendering/vulkan/vkr_texture.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_swapchain.h"
#include "rendering/r_constants.h"
#include "math/types.h"
#include "allocator/allocator.h"
#include "common/profiler.h"
#include "common/cvar.h"
#include <string.h>

typedef struct vkrScreenBlit_s
{
    vkrBuffer meshbuf;
    vkrBufferSet stagebuf;
    vkrImageSet image;
} vkrScreenBlit;
static vkrScreenBlit ms_blit;

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

bool vkrScreenBlit_New(VkRenderPass renderPass)
{
    vkrScreenBlit *const blit = &ms_blit;
    memset(blit, 0, sizeof(*blit));
    bool success = true;

    if (!vkrBuffer_New(
        &blit->meshbuf,
        sizeof(kScreenMesh),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        vkrMemUsage_CpuToGpu))
    {
        ASSERT(false);
        success = false;
        goto cleanup;
    }

cleanup:
    if (!success)
    {
        vkrScreenBlit_Del();
    }
    return success;
}

void vkrScreenBlit_Del(void)
{
    vkrImageSet_Release(&ms_blit.image);
    vkrBuffer_Release(&ms_blit.meshbuf);
    vkrBufferSet_Release(&ms_blit.stagebuf);
    memset(&ms_blit, 0, sizeof(ms_blit));
}

ProfileMark(pm_blit, vkrScreenBlit_Blit)
void vkrScreenBlit_Blit(
    const vkrPassContext* passCtx,
    const void* src,
    i32 width,
    i32 height,
    VkFormat format)
{
    ProfileBegin(pm_blit);

    ASSERT(passCtx);
    ASSERT(src);
    const i32 srcBytes = (width * height * vkrFormatToBpp(format)) / 8;
    ASSERT(srcBytes > 0);

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
        .usage =
            VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = NELEM(queueFamilies),
        .pQueueFamilyIndices = queueFamilies,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    if (!vkrImageSet_Reserve(
        &ms_blit.image,
        &imageInfo,
        vkrMemUsage_GpuOnly))
    {
        ASSERT(false);
        goto cleanup;
    }
    if (!vkrBufferSet_Reserve(
        &ms_blit.stagebuf,
        srcBytes,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        vkrMemUsage_CpuOnly))
    {
        ASSERT(false);
        goto cleanup;
    }

    const u32 imageIndex = vkrSys_SwapIndex();
    vkrSwapchain const *const chain = &g_vkr.chain;
    VkCommandBuffer cmd = passCtx->cmd;
    VkImage dstImage = chain->images[imageIndex];
    vkrImage* srcImage = vkrImageSet_Current(&ms_blit.image);
    vkrBuffer* stageBuf = vkrBufferSet_Current(&ms_blit.stagebuf);

    vkrBuffer_Write(stageBuf, src, srcBytes);
    vkrImage_Barrier(
        srcImage,
        cmd,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_ACCESS_TRANSFER_READ_BIT,
        VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT);

    // copy buffer to src image
    {
        const VkBufferImageCopy bufferRegion =
        {
            .imageSubresource =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .imageExtent.width = width,
            .imageExtent.height = height,
            .imageExtent.depth = 1,
        };
        vkCmdCopyBufferToImage(
            cmd,
            stageBuf->handle,
            srcImage->handle,
            srcImage->layout,
            1, &bufferRegion);
    }
    vkrImage_Barrier(
        srcImage,
        cmd,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_ACCESS_TRANSFER_READ_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT);

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
            .srcOffsets[0] = { 0, height, 0 }, // flipped vertically
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
            srcImage->handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
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

cleanup:
    ProfileEnd(pm_blit);
}
