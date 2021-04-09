#include "rendering/vulkan/vkr_texture.h"
#include "rendering/vulkan/vkr_image.h"
#include "rendering/vulkan/vkr_context.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_buffer.h"
#include "rendering/vulkan/vkr_mem.h"
#include "rendering/vulkan/vkr_textable.h"
#include "rendering/vulkan/vkr_sampler.h"
#include "allocator/allocator.h"
#include "common/time.h"
#include "math/scalar.h"
#include <string.h>

i32 vkrFormatToBpp(VkFormat format)
{
    if (format <= VK_FORMAT_UNDEFINED)
    {
        ASSERT(false);
        return 0;
    }
    if (format <= VK_FORMAT_R4G4_UNORM_PACK8)
    {
        return 8;
    }
    if (format <= VK_FORMAT_A1R5G5B5_UNORM_PACK16)
    {
        return 16;
    }
    if (format <= VK_FORMAT_R8_SRGB)
    {
        return 1 * 8;
    }
    if (format <= VK_FORMAT_R8G8_SRGB)
    {
        return 2 * 8;
    }
    if (format <= VK_FORMAT_B8G8R8_SRGB)
    {
        return 3 * 8;
    }
    if (format <= VK_FORMAT_B8G8R8A8_SRGB)
    {
        return 4 * 8;
    }
    if (format <= VK_FORMAT_A2B10G10R10_SINT_PACK32)
    {
        return 32;
    }
    if (format <= VK_FORMAT_R16_SFLOAT)
    {
        return 1 * 16;
    }
    if (format <= VK_FORMAT_R16G16_SFLOAT)
    {
        return 2 * 16;
    }
    if (format <= VK_FORMAT_R16G16B16_SFLOAT)
    {
        return 3 * 16;
    }
    if (format <= VK_FORMAT_R16G16B16A16_SFLOAT)
    {
        return 4 * 16;
    }
    if (format <= VK_FORMAT_R32_SFLOAT)
    {
        return 1 * 32;
    }
    if (format <= VK_FORMAT_R32G32_SFLOAT)
    {
        return 2 * 32;
    }
    if (format <= VK_FORMAT_R32G32B32_SFLOAT)
    {
        return 3 * 32;
    }
    if (format <= VK_FORMAT_R32G32B32A32_SFLOAT)
    {
        return 4 * 32;
    }
    if (format <= VK_FORMAT_R64_SFLOAT)
    {
        return 1 * 64;
    }
    if (format <= VK_FORMAT_R64G64_SFLOAT)
    {
        return 2 * 64;
    }
    if (format <= VK_FORMAT_R64G64B64_SFLOAT)
    {
        return 3 * 64;
    }
    if (format <= VK_FORMAT_R64G64B64A64_SFLOAT)
    {
        return 4 * 64;
    }
    switch (format)
    {
    default: ASSERT(false); return 0;
    case VK_FORMAT_B10G11R11_UFLOAT_PACK32: return 10 + 11 + 11;
    case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32: return 5 + 9 + 9 + 9;
    case VK_FORMAT_D16_UNORM: return 16;
    case VK_FORMAT_X8_D24_UNORM_PACK32: return 8 + 24;
    case VK_FORMAT_D32_SFLOAT: return 32;
    case VK_FORMAT_S8_UINT: return 8;
    case VK_FORMAT_D16_UNORM_S8_UINT: return 16 + 8;
    case VK_FORMAT_D32_SFLOAT_S8_UINT: return 32 + 8;
    }
}

static VkImageType ViewTypeToImageType(VkImageViewType viewType)
{
    switch (viewType)
    {
    default:
        ASSERT(false);
        return VK_IMAGE_TYPE_2D;
    case VK_IMAGE_VIEW_TYPE_1D:
        return VK_IMAGE_TYPE_1D;
    case VK_IMAGE_VIEW_TYPE_2D:
        return VK_IMAGE_TYPE_2D;
    case VK_IMAGE_VIEW_TYPE_3D:
        return VK_IMAGE_TYPE_3D;
    case VK_IMAGE_VIEW_TYPE_CUBE:
        return VK_IMAGE_TYPE_2D;
    case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
        return VK_IMAGE_TYPE_1D;
    case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
        return VK_IMAGE_TYPE_2D;
    case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
    {
        ASSERT(g_vkrFeats.phdev.features.imageCubeArray);
        return VK_IMAGE_TYPE_2D;
    }
    }
}

static VkImageCreateFlags ViewTypeToCreateFlags(VkImageViewType viewType)
{
    switch (viewType)
    {
    default:
        return 0x0;
    case VK_IMAGE_VIEW_TYPE_CUBE:
        return VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
        return VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }
}

i32 vkrTexture_MipCount(i32 width, i32 height, i32 depth)
{
    i32 m = i1_max(width, i1_max(height, depth));
    return 1 + (i32)floorf(log2f((float)m));
}

bool vkrTexture_New(
    vkrImage *const image,
    VkImageViewType viewType,
    VkFormat format,
    i32 width,
    i32 height,
    i32 depth,
    i32 layers,
    bool mips)
{
    memset(image, 0, sizeof(*image));
    if (width * height * depth <= 0)
    {
        ASSERT(false);
        return false;
    }
    const i32 mipCount = mips ? vkrTexture_MipCount(width, height, depth) : 1;
    const u32 queueFamilies[] =
    {
        g_vkr.queues[vkrQueueId_Gfx].family,
    };
    VkImageCreateInfo info = { 0 };
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.flags = ViewTypeToCreateFlags(viewType);
    info.imageType = ViewTypeToImageType(viewType);
    info.format = format;
    info.extent.width = width;
    info.extent.height = height;
    info.extent.depth = depth;
    info.mipLevels = mipCount;
    info.arrayLayers = layers;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage =
        VK_IMAGE_USAGE_TRANSFER_DST_BIT |
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
        VK_IMAGE_USAGE_SAMPLED_BIT;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.queueFamilyIndexCount = NELEM(queueFamilies);
    info.pQueueFamilyIndices = queueFamilies;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    return vkrImage_New(
        image,
        &info,
        vkrMemUsage_GpuOnly);
}

void vkrTexture_Release(vkrImage *const image)
{
    vkrImage_Release(image);
}

VkFence vkrTexture_Upload(
    vkrImage *const image,
    i32 layer,
    void const *const data,
    i32 bytes)
{
    ASSERT(image);
    ASSERT(image->handle);
    ASSERT(bytes >= 0);
    ASSERT(data || !bytes);
    ASSERT(layer >= 0);
    ASSERT(layer < image->arrayLayers);
    if (bytes <= 0)
    {
        return NULL;
    }

    vkrBuffer stagebuf = { 0 };
    if (!vkrBuffer_New(
        &stagebuf,
        bytes,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        vkrMemUsage_CpuOnly))
    {
        return NULL;
    }

    {
        void *const dst = vkrBuffer_Map(&stagebuf);
        ASSERT(dst);
        if (dst)
        {
            memcpy(dst, data, bytes);
        }
        vkrBuffer_Unmap(&stagebuf);
        vkrBuffer_Flush(&stagebuf);
    }

    const i32 width = image->width;
    const i32 height = image->height;
    const i32 depth = image->depth;
    const i32 mipCount = image->mipLevels;
    VkImage handle = image->handle;

    VkFence fence = NULL;
    VkQueue queue = NULL;
    VkCommandBuffer cmd = vkrContext_GetTmpCmd(vkrQueueId_Gfx, &fence, &queue);
    vkrCmdBegin(cmd);
    {
        // transition all mips from undefined to xfer dst
        VkImageMemoryBarrier barrier =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = handle,
            .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = VK_REMAINING_MIP_LEVELS,
                .baseArrayLayer = layer,
                .layerCount = 1,
            },
        };
        vkrCmdImageBarrier(
            cmd,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            &barrier);
        image->layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

        // copy buffer to image mip 0
        const VkBufferImageCopy region =
        {
            .imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .imageSubresource.mipLevel = 0,
            .imageSubresource.baseArrayLayer = layer,
            .imageSubresource.layerCount = 1,
            .imageExtent.width = width,
            .imageExtent.height = height,
            .imageExtent.depth = depth,
        };
        vkCmdCopyBufferToImage(
            cmd,
            stagebuf.handle,
            handle,
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
            i32 srcDepth = i1_max(depth >> (i - 1), 1);
            i32 dstWidth = i1_max(width >> i, 1);
            i32 dstHeight = i1_max(height >> i, 1);
            i32 dstDepth = i1_max(depth >> i, 1);
            const VkImageBlit blit =
            {
                .srcOffsets[1] = { srcWidth, srcHeight, srcDepth, },
                .dstOffsets[1] = { dstWidth, dstHeight, dstDepth, },
                .srcSubresource =
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = i - 1,
                    .baseArrayLayer = layer,
                    .layerCount = 1,
                },
                .dstSubresource =
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = i,
                    .baseArrayLayer = layer,
                    .layerCount = 1,
                },
            };
            vkCmdBlitImage(
                cmd,
                handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
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
    vkrBuffer_Release(&stagebuf);

    image->layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    return fence;
}
