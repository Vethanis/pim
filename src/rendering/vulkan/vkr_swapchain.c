#include "rendering/vulkan/vkr_swapchain.h"
#include "rendering/vulkan/vkr_display.h"
#include "rendering/vulkan/vkr_queue.h"
#include "rendering/vulkan/vkr_sync.h"
#include "rendering/vulkan/vkr_device.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_mem.h"
#include "rendering/vulkan/vkr_image.h"
#include "rendering/vulkan/vkr_attachment.h"
#include "rendering/vulkan/vkr_context.h"
#include "rendering/vulkan/vkr_renderpass.h"
#include "rendering/vulkan/vkr_framebuffer.h"
#include "allocator/allocator.h"
#include "common/console.h"
#include "common/profiler.h"
#include "math/scalar.h"
#include <GLFW/glfw3.h>
#include <string.h>

// ----------------------------------------------------------------------------

static void vkrSwapchain_SetupBuffers(vkrSwapchain* chain);

// ----------------------------------------------------------------------------

static const char* VkPresentModeKHR_Str[] =
{
    "VK_PRESENT_MODE_IMMEDIATE_KHR",
    "VK_PRESENT_MODE_MAILBOX_KHR",
    "VK_PRESENT_MODE_FIFO_KHR",
    "VK_PRESENT_MODE_FIFO_RELAXED_KHR",
};

// pim prefers low latency over tear protection
static const VkPresentModeKHR kPreferredPresentModes[] =
{
    VK_PRESENT_MODE_MAILBOX_KHR,        // single-entry overwrote queue; good latency
    VK_PRESENT_MODE_IMMEDIATE_KHR,      // no queue; best latency, bad tearing
    VK_PRESENT_MODE_FIFO_RELAXED_KHR,   // multi-entry adaptive queue; bad latency
    VK_PRESENT_MODE_FIFO_KHR ,          // multi-entry queue; bad latency
};

static const VkSurfaceFormatKHR kPreferredSurfaceFormats[] =
{
#if ENABLE_HDR
    // PQ Rec2100
    // https://en.wikipedia.org/wiki/Rec._2100
    // https://en.wikipedia.org/wiki/High-dynamic-range_video#Perceptual_quantizer
    {
        VK_FORMAT_A2R10G10B10_UNORM_PACK32,
        VK_COLOR_SPACE_HDR10_ST2084_EXT,
    },
    {
        VK_FORMAT_A2B10G10R10_UNORM_PACK32,
        VK_COLOR_SPACE_HDR10_ST2084_EXT,
    },
    // 10 bit sRGB
    {
        VK_FORMAT_A2R10G10B10_UNORM_PACK32,
        VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
    },
    {
        VK_FORMAT_A2B10G10R10_UNORM_PACK32,
        VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
    },
#endif // ENABLE_HDR
    // 8 bit sRGB
    {
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
    },
    {
        VK_FORMAT_B8G8R8A8_SRGB,
        VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
    },
};

// ----------------------------------------------------------------------------

bool vkrSwapchain_New(
    vkrSwapchain* chain,
    vkrWindow* window,
    vkrSwapchain* prev)
{
    ASSERT(chain);
    ASSERT(window);
    memset(chain, 0, sizeof(*chain));

    VkPhysicalDevice phdev = g_vkr.phdev;
    VkDevice dev = g_vkr.dev;
    VkSurfaceKHR surface = window->surface;
    ASSERT(phdev);
    ASSERT(dev);
    ASSERT(surface);

    vkrSwapchainSupport sup = vkrQuerySwapchainSupport(phdev, surface);
    vkrQueueSupport qsup = vkrQueryQueueSupport(phdev, surface);

    VkSurfaceFormatKHR format = vkrSelectSwapFormat(sup.formats, sup.formatCount);
    VkPresentModeKHR mode = vkrSelectSwapMode(sup.modes, sup.modeCount);
    VkExtent2D ext = vkrSelectSwapExtent(&sup.caps, window->width, window->height);

    u32 imgCount = i1_clamp(kDesiredSwapchainLen, sup.caps.minImageCount, i1_min(kMaxSwapchainLen, sup.caps.maxImageCount));

    const u32 families[] =
    {
        qsup.family[vkrQueueId_Graphics],
        qsup.family[vkrQueueId_Present],
    };
    bool concurrent = families[0] != families[1];

    const VkSwapchainCreateInfoKHR swapInfo =
    {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .presentMode = mode,
        .minImageCount = imgCount,
        .imageFormat = format.format,
        .imageColorSpace = format.colorSpace,
        .imageExtent = ext,
        .imageArrayLayers = 1,
        // use transfer_dst_bit if rendering offscreen
        .imageUsage =
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode = concurrent ?
            VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = concurrent ? NELEM(families) : 0,
        .pQueueFamilyIndices = families,
        .preTransform = sup.caps.currentTransform,
        // no compositing with window manager / desktop background
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        // don't render pixels behind other windows
        .clipped = VK_TRUE,
        // prev swapchain, if re-creating
        .oldSwapchain = prev ? prev->handle : NULL,
    };

    VkSwapchainKHR handle = NULL;
    VkCheck(vkCreateSwapchainKHR(dev, &swapInfo, NULL, &handle));
    ASSERT(handle);
    if (!handle)
    {
        Con_Logf(LogSev_Error, "Vk", "Failed to create swapchain, null handle");
        return false;
    }

    chain->handle = handle;
    chain->mode = mode;
    chain->colorFormat = format.format;
    chain->colorSpace = format.colorSpace;
    chain->width = ext.width;
    chain->height = ext.height;

    if (!prev)
    {
        Con_Logf(LogSev_Info, "vkr", "Present mode: '%s'", VkPresentModeKHR_Str[mode]);
        Con_Logf(LogSev_Info, "vkr", "Present extent: %u x %u", ext.width, ext.height);
        Con_Logf(LogSev_Info, "vkr", "Present images: %u", imgCount);
        Con_Logf(LogSev_Info, "vkr", "Present sharing mode: %s", concurrent ? "Concurrent" : "Exclusive");
        const char* colorSpaceStr = "Unknown";
        switch (format.colorSpace)
        {
        default:
            break;
        case VK_COLORSPACE_SRGB_NONLINEAR_KHR:
            colorSpaceStr = "sRGB Gamma";
            break;
        case VK_COLOR_SPACE_HDR10_ST2084_EXT:
            colorSpaceStr = "Rec2100 PQ";
            break;
        }
        Con_Logf(LogSev_Info, "vkr", "Color space: %s", colorSpaceStr);
        const char* formatStr = "Unknown";
        switch (format.format)
        {
        default:
            break;
        case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
            formatStr = "A2R10G10B10";
            break;
        case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
            formatStr = "A2B10G10R10";
            break;
        case VK_FORMAT_R8G8B8A8_SRGB:
            formatStr = "R8G8B8A8_SRGB";
            break;
        case VK_FORMAT_B8G8R8A8_SRGB:
            formatStr = "B8G8R8A8_SRGB";
            break;
        }
        Con_Logf(LogSev_Info, "vkr", "Format: %s", formatStr);
    }

    // get swapchain images
    {
        VkCheck(vkGetSwapchainImagesKHR(dev, handle, &imgCount, NULL));
        if (imgCount > kMaxSwapchainLen)
        {
            ASSERT(false);
            vkDestroySwapchainKHR(dev, handle, NULL);
            memset(chain, 0, sizeof(*chain));
            return false;
        }
        chain->length = imgCount;
        VkCheck(vkGetSwapchainImagesKHR(dev, handle, &imgCount, chain->images));
    }

    // create image views
    for (u32 i = 0; i < imgCount; ++i)
    {
        const VkImageViewCreateInfo viewInfo =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = chain->images[i],
            .format = chain->colorFormat,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.levelCount = 1,
            .subresourceRange.layerCount = 1,
        };
        VkImageView view = NULL;
        VkCheck(vkCreateImageView(dev, &viewInfo, NULL, &view));
        ASSERT(view);
        chain->views[i] = view;
    }

    // create luminance attachments
    for(u32 i = 0; i < imgCount; ++i)
    {
        vkrAttachment_New(
            &chain->depthAttachments[i],
            chain->width,
            chain->height,
            VK_FORMAT_D32_SFLOAT,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
        vkrAttachment_New(
            &chain->lumAttachments[i],
            chain->width,
            chain->height,
            VK_FORMAT_R16_SFLOAT,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT);
    }

    // create synchronization objects
    for (i32 i = 0; i < kResourceSets; ++i)
    {
        chain->syncFences[i] = vkrFence_New(true);
        chain->availableSemas[i] = vkrSemaphore_New();
        chain->renderedSemas[i] = vkrSemaphore_New();
    }

    // create presentation command buffers
    const VkCommandPoolCreateInfo cmdpoolinfo =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = g_vkr.queues[vkrQueueId_Graphics].family,
    };
    VkCommandPool cmdpool = NULL;
    VkCheck(vkCreateCommandPool(g_vkr.dev, &cmdpoolinfo, NULL, &cmdpool));
    ASSERT(cmdpool);
    chain->cmdpool = cmdpool;
    const VkCommandBufferAllocateInfo cmdinfo =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = NELEM(chain->presCmds),
        .commandPool = cmdpool,
    };
    VkCheck(vkAllocateCommandBuffers(g_vkr.dev, &cmdinfo, chain->presCmds));

    vkrSwapchain_SetupBuffers(chain);

    return true;
}

void vkrSwapchain_Del(vkrSwapchain* chain)
{
    if (chain)
    {
        vkrDevice_WaitIdle();
        vkrContext_OnSwapDel(&g_vkr.context);

        if (chain->cmdpool)
        {
            vkDestroyCommandPool(g_vkr.dev, chain->cmdpool, NULL);
            chain->cmdpool = NULL;
        }

        for (i32 i = 0; i < kResourceSets; ++i)
        {
            vkrFence_Del(chain->syncFences[i]);
            vkrSemaphore_Del(chain->availableSemas[i]);
            vkrSemaphore_Del(chain->renderedSemas[i]);
            chain->syncFences[i] = NULL;
            chain->availableSemas[i] = NULL;
            chain->renderedSemas[i] = NULL;
            chain->presCmds[i] = NULL;
        }

        const i32 len = chain->length;
        for (i32 i = 0; i < len; ++i)
        {
            vkrFramebuffer_Del(&chain->buffers[i]);
            vkrImageView_Release(chain->views[i]);
            vkrAttachment_Release(&chain->lumAttachments[i]);
            vkrAttachment_Release(&chain->depthAttachments[i]);
        }

        if (chain->handle)
        {
            vkDestroySwapchainKHR(g_vkr.dev, chain->handle, NULL);
            chain->handle = NULL;
        }

        memset(chain, 0, sizeof(*chain));
    }
}

static void vkrSwapchain_SetupBuffers(vkrSwapchain* chain)
{
    ASSERT(chain);
    const vkrRenderPassDesc renderPassDesc =
    {
        .srcAccessMask =
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask =
            VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,

        .srcStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,

        .attachments[0] =
        {
            .format = chain->depthAttachments[0].format,
            .initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .load = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .store = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        },
        .attachments[1] =
        {
            .format = chain->colorFormat,
            .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .load = VK_ATTACHMENT_LOAD_OP_LOAD,
            .store = VK_ATTACHMENT_STORE_OP_STORE,
        },
        .attachments[2] =
        {
            .format = chain->lumAttachments[0].format,
            .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .load = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .store = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        },
    };
    chain->presentPass = vkrRenderPass_Get(&renderPassDesc);
    ASSERT(chain->presentPass);

    const i32 len = chain->length;
    for (i32 i = 0; i < len; ++i)
    {
        vkrFramebuffer_Del(&chain->buffers[i]);
        const VkImageView attachments[] =
        {
            chain->depthAttachments[i].view,
            chain->views[i],
            chain->lumAttachments[i].view,
        };
        const VkFormat formats[] =
        {
            chain->depthAttachments[i].format,
            chain->colorFormat,
            chain->lumAttachments[i].format,
        };
        vkrFramebuffer_New(
            &chain->buffers[i],
            attachments, formats, NELEM(attachments),
            chain->width,
            chain->height);
    }
}

bool vkrSwapchain_Recreate(
    vkrSwapchain* chain,
    vkrWindow* window)
{
    ASSERT(chain);
    ASSERT(window);
    vkrSwapchain next = { 0 };
    vkrSwapchain prev = *chain;
    bool recreated = false;
    if ((window->width > 0) && (window->height > 0))
    {
        recreated = true;
        vkrSwapchain_New(&next, window, &prev);
        vkrSwapchain_SetupBuffers(&next);
    }
    vkrSwapchain_Del(&prev);
    *chain = next;
    return recreated;
}

ProfileMark(pm_acquiresync, vkrSwapchain_AcquireSync)
u32 vkrSwapchain_AcquireSync(vkrSwapchain* chain, VkCommandBuffer* cmdOut, VkFence* fenceOut)
{
    ProfileBegin(pm_acquiresync);

    ASSERT(chain);
    ASSERT(chain->handle);
    ASSERT(cmdOut);
    ASSERT(fenceOut);

    u32 syncIndex = (chain->syncIndex + 1) % kResourceSets;
    VkFence fence = chain->syncFences[syncIndex];
    VkCommandBuffer cmd = chain->presCmds[syncIndex];
    ASSERT(fence);
    ASSERT(cmd);
    // acquire resources associated with this frame in flight
    vkrFence_Wait(fence);
    chain->syncIndex = syncIndex;

    VkCheck(vkResetCommandBuffer(cmd, 0x0));

    vkrCmdBegin(cmd);

    ProfileEnd(pm_acquiresync);

    *cmdOut = cmd;
    *fenceOut = fence;
    return syncIndex;
}

ProfileMark(pm_acquireimg, vkrSwapchain_AcquireImage)
u32 vkrSwapchain_AcquireImage(vkrSwapchain* chain, VkFramebuffer* bufferOut)
{
    ProfileBegin(pm_acquireimg);

    ASSERT(chain);
    ASSERT(chain->handle);
    ASSERT(chain->length > 0);
    ASSERT(bufferOut);

    u32 syncIndex = chain->syncIndex;
    ASSERT(syncIndex < kResourceSets);

    u32 imageIndex = 0;
    const u64 timeout = -1;
    VkCheck(vkAcquireNextImageKHR(
        g_vkr.dev,
        chain->handle,
        timeout,
        chain->availableSemas[syncIndex],
        NULL,
        &imageIndex));

    ASSERT(imageIndex < (u32)chain->length);
    {
        // acquire resources associated with the frame in flight for this image
        // driver might not use same ring buffer index as the application
        VkFence fence = chain->imageFences[imageIndex];
        if (fence)
        {
            vkrFence_Wait(fence);
        }
    }
    chain->imageFences[imageIndex] = chain->syncFences[syncIndex];
    chain->imageIndex = imageIndex;

    ProfileEnd(pm_acquireimg);

    *bufferOut = chain->buffers[imageIndex].handle;
    return imageIndex;
}

ProfileMark(pm_submit, vkrSwapchain_Submit)
void vkrSwapchain_Submit(vkrSwapchain* chain, VkCommandBuffer cmd)
{
    ProfileBegin(pm_submit);

    ASSERT(chain);
    ASSERT(chain->handle);
    ASSERT(cmd);

    u32 imageIndex = chain->imageIndex;
    u32 syncIndex = chain->syncIndex;
    ASSERT(cmd == chain->presCmds[syncIndex]);
    VkQueue gfxQueue = g_vkr.queues[vkrQueueId_Graphics].handle;
    ASSERT(gfxQueue);

    const VkClearValue clearValues[] =
    {
        {
            .depthStencil = { 1.0f, 0 },
        },
        {
            .color = { 0.0f, 0.0f, 0.0f, 1.0f },
        },
        {
            .color = { 0.0f, 0.0f, 0.0f, 1.0f },
        },
    };
    vkrCmdBeginRenderPass(
        cmd,
        chain->presentPass,
        chain->buffers[imageIndex].handle,
        vkrSwapchain_GetRect(chain),
        NELEM(clearValues), clearValues,
        VK_SUBPASS_CONTENTS_INLINE);
    vkrCmdEndRenderPass(cmd);
    vkrCmdEnd(cmd);

    vkrFence_Reset(chain->syncFences[syncIndex]);
    vkrCmdSubmit(
        gfxQueue,
        cmd,
        chain->syncFences[syncIndex],
        chain->availableSemas[syncIndex],
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        chain->renderedSemas[syncIndex]);

    ProfileEnd(pm_submit);
}

ProfileMark(pm_present, vkrSwapchain_Present)
void vkrSwapchain_Present(vkrSwapchain* chain)
{
    ASSERT(chain);
    ASSERT(chain->handle);

    u32 syncIndex = chain->syncIndex;
    VkQueue presentQueue = g_vkr.queues[vkrQueueId_Present].handle;
    ASSERT(presentQueue);

    ProfileBegin(pm_present);

    const VkPresentInfoKHR presentInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &chain->renderedSemas[syncIndex],
        .swapchainCount = 1,
        .pSwapchains = &chain->handle,
        .pImageIndices = &chain->imageIndex,
    };
    VkCheck(vkQueuePresentKHR(presentQueue, &presentInfo));

    ProfileEnd(pm_present);
}

VkViewport vkrSwapchain_GetViewport(const vkrSwapchain* chain)
{
    ASSERT(chain);
    VkViewport viewport =
    {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)chain->width,
        .height = (float)chain->height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    return viewport;
}

VkRect2D vkrSwapchain_GetRect(const vkrSwapchain* chain)
{
    ASSERT(chain);
    VkRect2D rect =
    {
        .extent.width = chain->width,
        .extent.height = chain->height,
    };
    return rect;
}

float vkrSwapchain_GetAspect(const vkrSwapchain* chain)
{
    return (float)chain->width / (float)chain->height;
}

// ----------------------------------------------------------------------------

vkrSwapchainSupport vkrQuerySwapchainSupport(
    VkPhysicalDevice phdev,
    VkSurfaceKHR surf)
{
    ASSERT(phdev);
    ASSERT(surf);

    vkrSwapchainSupport sup = { 0 };

    VkCheck(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phdev, surf, &sup.caps));

    VkCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(phdev, surf, &sup.formatCount, NULL));
    sup.formats = Temp_Calloc(sizeof(sup.formats[0]) * sup.formatCount);
    VkCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(phdev, surf, &sup.formatCount, sup.formats));

    VkCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(phdev, surf, &sup.modeCount, NULL));
    sup.modes = Temp_Calloc(sizeof(sup.modes[0]) * sup.modeCount);
    VkCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(phdev, surf, &sup.modeCount, sup.modes));

    return sup;
}

VkSurfaceFormatKHR vkrSelectSwapFormat(
    const VkSurfaceFormatKHR* formats,
    i32 count)
{
    ASSERT(formats || !count);
    for (i32 p = 0; p < NELEM(kPreferredSurfaceFormats); ++p)
    {
        VkSurfaceFormatKHR pref = kPreferredSurfaceFormats[p];
        for (i32 i = 0; i < count; ++i)
        {
            VkSurfaceFormatKHR sf = formats[i];
            if (memcmp(&sf, &pref, sizeof(sf)) == 0)
            {
                return sf;
            }
        }
    }
    if (count > 0)
    {
        return formats[0];
    }
    return (VkSurfaceFormatKHR) { 0 };
}

VkPresentModeKHR vkrSelectSwapMode(
    const VkPresentModeKHR* modes,
    i32 count)
{
    ASSERT(modes || !count);
    for (i32 p = 0; p < NELEM(kPreferredPresentModes); ++p)
    {
        VkPresentModeKHR pref = kPreferredPresentModes[p];
        for (i32 i = 0; i < count; ++i)
        {
            VkPresentModeKHR mode = modes[i];
            if (pref == mode)
            {
                return mode;
            }
        }
    }
    if (count > 0)
    {
        return modes[0];
    }
    return VK_PRESENT_MODE_IMMEDIATE_KHR;
}

VkExtent2D vkrSelectSwapExtent(
    const VkSurfaceCapabilitiesKHR* caps,
    i32 width,
    i32 height)
{
    if (caps->currentExtent.width != ~0u)
    {
        return caps->currentExtent;
    }
    VkExtent2D minExt = caps->minImageExtent;
    VkExtent2D maxExt = caps->maxImageExtent;
    VkExtent2D ext = { 0 };
    ext.width = i1_clamp(width, minExt.width, maxExt.width);
    ext.height = i1_clamp(height, minExt.height, maxExt.height);
    return ext;
}
