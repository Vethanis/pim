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

static vkrBufferSet ms_stagebuf;
static vkrImageSet ms_image;

// ----------------------------------------------------------------------------

bool vkrScreenBlit_New(void)
{
    return true;
}

void vkrScreenBlit_Del(void)
{
    vkrBufferSet_Release(&ms_stagebuf);
    vkrImageSet_Release(&ms_image);
}

ProfileMark(pm_blit, vkrScreenBlit_Blit)
void vkrScreenBlit_Blit(
    const void* src,
    i32 width,
    i32 height,
    VkFormat format)
{
    ProfileBegin(pm_blit);

    ASSERT(src);
    const i32 srcBytes = (width * height * vkrFormatToBpp(format)) / 8;
    ASSERT(srcBytes > 0);

    const u32 queueFamilies[] =
    {
        g_vkr.queues[vkrQueueId_Graphics].family,
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
        &ms_image,
        &imageInfo,
        vkrMemUsage_GpuOnly))
    {
        ASSERT(false);
        goto cleanup;
    }
    if (!vkrBufferSet_Reserve(
        &ms_stagebuf,
        srcBytes,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        vkrMemUsage_CpuOnly))
    {
        ASSERT(false);
        goto cleanup;
    }

    vkrCmdBuf* cmd = vkrCmdGet_G();
    vkrImage* srcImage = vkrImageSet_Current(&ms_image);
    vkrImage* dstImage = vkrGetSceneBuffer();
    vkrBuffer* stageBuf = vkrBufferSet_Current(&ms_stagebuf);

    vkrBufferState_HostWrite(cmd, stageBuf);
    vkrBuffer_Write(stageBuf, src, srcBytes);

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
    vkrCmdCopyBufferToImage(
        cmd,
        stageBuf,
        srcImage,
        &bufferRegion);

    const VkImageBlit imageRegion =
    {
        .srcOffsets[0] = { 0, height, 0 }, // flipped vertically
        .srcOffsets[1] = { width, 0, 1 },
        .dstOffsets[1] = { dstImage->width, dstImage->height, 1 },
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
    vkrCmdBlitImage(
        cmd,
        srcImage,
        dstImage,
        &imageRegion);

cleanup:
    ProfileEnd(pm_blit);
}
