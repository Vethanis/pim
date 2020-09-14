#include "rendering/vulkan/vkr_swapchain.h"
#include "rendering/vulkan/vkr_display.h"
#include "rendering/vulkan/vkr_queue.h"
#include "rendering/vulkan/vkr_sync.h"
#include "rendering/vulkan/vkr_device.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_mem.h"
#include "rendering/vulkan/vkr_image.h"
#include "rendering/vulkan/vkr_attachment.h"
#include "allocator/allocator.h"
#include "common/console.h"
#include "common/profiler.h"
#include "math/scalar.h"
#include <GLFW/glfw3.h>
#include <string.h>

bool vkrSwapchain_New(
    vkrSwapchain* chain,
    vkrDisplay* display,
    vkrSwapchain* prev)
{
    ASSERT(chain);
    ASSERT(display);
    memset(chain, 0, sizeof(*chain));

    VkPhysicalDevice phdev = g_vkr.phdev;
    VkDevice dev = g_vkr.dev;
    VkSurfaceKHR surface = display->surface;
    ASSERT(phdev);
    ASSERT(dev);
    ASSERT(surface);

    vkrSwapchainSupport sup = vkrQuerySwapchainSupport(phdev, surface);
    vkrQueueSupport qsup = vkrQueryQueueSupport(phdev, surface);

    VkSurfaceFormatKHR format = vkrSelectSwapFormat(sup.formats, sup.formatCount);
    VkPresentModeKHR mode = vkrSelectSwapMode(sup.modes, sup.modeCount);
    VkExtent2D ext = vkrSelectSwapExtent(&sup.caps, display->width, display->height);

    u32 imgCount = i1_clamp(kDesiredSwapchainLen, sup.caps.minImageCount, i1_min(kMaxSwapchainLen, sup.caps.maxImageCount));

    const u32 families[] =
    {
        qsup.family[vkrQueueId_Gfx],
        qsup.family[vkrQueueId_Pres],
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
        con_logf(LogSev_Error, "Vk", "Failed to create swapchain, null handle");
        return false;
    }

    chain->handle = handle;
    chain->mode = mode;
    chain->colorFormat = format.format;
    chain->colorSpace = format.colorSpace;
    chain->width = ext.width;
    chain->height = ext.height;

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

    // create depth attachment
    {
        vkrAttachment_New(
            &chain->depthAttachment,
            chain->width,
            chain->height,
            VK_FORMAT_D32_SFLOAT,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }

    // create luminance attachments
    for(u32 i = 0; i < imgCount; ++i)
    {
        vkrAttachment_New(
            &chain->lumAttachments[i],
            chain->width,
            chain->height,
            VK_FORMAT_R16_SFLOAT,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT);
    }

    // create synchronization objects
    for (i32 i = 0; i < kFramesInFlight; ++i)
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
        .queueFamilyIndex = g_vkr.queues[vkrQueueId_Gfx].family,
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

    return true;
}

void vkrSwapchain_Del(vkrSwapchain* chain)
{
    if (chain)
    {
        vkDeviceWaitIdle(g_vkr.dev);

        if (chain->cmdpool)
        {
            vkDestroyCommandPool(g_vkr.dev, chain->cmdpool, NULL);
            chain->cmdpool = NULL;
        }

        for (i32 i = 0; i < kFramesInFlight; ++i)
        {
            vkrFence_Del(chain->syncFences[i]);
            vkrSemaphore_Del(chain->availableSemas[i]);
            vkrSemaphore_Del(chain->renderedSemas[i]);
            chain->syncFences[i] = NULL;
            chain->availableSemas[i] = NULL;
            chain->renderedSemas[i] = NULL;
            chain->presCmds[i] = NULL;
        }

        vkrAttachment_Del(&chain->depthAttachment);

        const i32 len = chain->length;
        for (i32 i = 0; i < len; ++i)
        {
            VkFramebuffer buffer = chain->buffers[i];
            if (buffer)
            {
                vkDestroyFramebuffer(g_vkr.dev, buffer, NULL);
            }
            vkrImageView_Del(chain->views[i]);
            vkrAttachment_Del(&chain->lumAttachments[i]);
        }

        if (chain->handle)
        {
            vkDestroySwapchainKHR(g_vkr.dev, chain->handle, NULL);
            chain->handle = NULL;
        }

        memset(chain, 0, sizeof(*chain));
    }
}

void vkrSwapchain_SetupBuffers(vkrSwapchain* chain, VkRenderPass presentPass)
{
    ASSERT(chain);
    ASSERT(presentPass);
    const i32 len = chain->length;
    for (i32 i = 0; i < len; ++i)
    {
        const VkImageView attachments[] =
        {
            chain->views[i],
            chain->lumAttachments[i].view,
            chain->depthAttachment.view,
        };
        VkFramebuffer buffer = chain->buffers[i];
        if (buffer)
        {
            vkDestroyFramebuffer(g_vkr.dev, buffer, NULL);
            buffer = NULL;
        }
        const VkFramebufferCreateInfo bufferInfo =
        {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = presentPass,
            .attachmentCount = NELEM(attachments),
            .pAttachments = attachments,
            .width = chain->width,
            .height = chain->height,
            .layers = 1,
        };
        VkCheck(vkCreateFramebuffer(g_vkr.dev, &bufferInfo, NULL, &buffer));
        ASSERT(buffer);

        chain->buffers[i] = buffer;
    }
}

bool vkrSwapchain_Recreate(
    vkrSwapchain* chain,
    vkrDisplay* display,
    VkRenderPass presentPass)
{
    ASSERT(chain);
    ASSERT(display);
    ASSERT(presentPass);
    vkrSwapchain next = { 0 };
    vkrSwapchain prev = *chain;
    vkrDevice_WaitIdle();
    bool recreated = false;
    if ((display->width > 0) && (display->height > 0))
    {
        recreated = true;
        vkrSwapchain_New(&next, display, &prev);
        vkrSwapchain_SetupBuffers(&next, presentPass);
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

    u32 syncIndex = (chain->syncIndex + 1) % kFramesInFlight;
    VkFence fence = chain->syncFences[syncIndex];
    VkCommandBuffer cmd = chain->presCmds[syncIndex];
    ASSERT(fence);
    ASSERT(cmd);
    // acquire resources associated with this frame in flight
    vkrFence_Wait(fence);
    chain->syncIndex = syncIndex;

    VkCheck(vkResetCommandBuffer(cmd, 0x0));

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
    ASSERT(syncIndex < kFramesInFlight);

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

    *bufferOut = chain->buffers[imageIndex];
    return imageIndex;
}

ProfileMark(pm_present, vkrSwapchain_Present)
void vkrSwapchain_Present(vkrSwapchain* chain)
{
    ASSERT(chain);
    ASSERT(chain->handle);

    u32 syncIndex = chain->syncIndex;
    VkCommandBuffer cmd = chain->presCmds[syncIndex];
    VkQueue gfxQueue = g_vkr.queues[vkrQueueId_Gfx].handle;
    VkQueue presentQueue = g_vkr.queues[vkrQueueId_Pres].handle;

    ASSERT(cmd);
    ASSERT(gfxQueue);
    ASSERT(presentQueue);

    vkrFence_Reset(chain->syncFences[syncIndex]);
    vkrCmdSubmit(
        gfxQueue,
        cmd,
        chain->syncFences[syncIndex],
        chain->availableSemas[syncIndex],
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        chain->renderedSemas[syncIndex]);

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
    sup.formats = tmp_calloc(sizeof(sup.formats[0]) * sup.formatCount);
    VkCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(phdev, surf, &sup.formatCount, sup.formats));

    VkCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(phdev, surf, &sup.modeCount, NULL));
    sup.modes = tmp_calloc(sizeof(sup.modes[0]) * sup.modeCount);
    VkCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(phdev, surf, &sup.modeCount, sup.modes));

    return sup;
}

static const VkSurfaceFormatKHR kPreferredSurfaceFormats[] =
{
    {
        VK_FORMAT_B8G8R8A8_SRGB,
        VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
    },
};

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

// pim prefers low latency over tear protection
static const VkPresentModeKHR kPreferredPresentModes[] =
{
    VK_PRESENT_MODE_MAILBOX_KHR,        // single-entry overwrote queue; good latency
    VK_PRESENT_MODE_IMMEDIATE_KHR,      // no queue; best latency, bad tearing
    VK_PRESENT_MODE_FIFO_RELAXED_KHR,   // multi-entry adaptive queue; bad latency
    VK_PRESENT_MODE_FIFO_KHR ,          // multi-entry queue; bad latency
};

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
