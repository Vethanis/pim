#include "rendering/vulkan/vkr_swapchain.h"
#include "rendering/vulkan/vkr_display.h"
#include "rendering/vulkan/vkr_queue.h"
#include "rendering/vulkan/vkr_sync.h"
#include "rendering/vulkan/vkr_device.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_mem.h"
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

    u32 imgCount = i1_clamp(3, sup.caps.minImageCount, i1_min(kMaxSwapchainLen, sup.caps.maxImageCount));

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
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
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
    VkCheck(vkCreateSwapchainKHR(dev, &swapInfo, g_vkr.alloccb, &handle));
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

    VkCheck(vkGetSwapchainImagesKHR(dev, handle, &imgCount, NULL));
    if (imgCount > kMaxSwapchainLen)
    {
        ASSERT(false);
        con_logf(LogSev_Error, "Vk", "Failed to create swapchain, too many images (%d of %d)", imgCount, kMaxSwapchainLen);
        vkDestroySwapchainKHR(dev, handle, g_vkr.alloccb);
        memset(chain, 0, sizeof(*chain));
        return false;
    }
    chain->length = imgCount;
    VkCheck(vkGetSwapchainImagesKHR(dev, handle, &imgCount, chain->images));

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
        VkCheck(vkCreateImageView(dev, &viewInfo, g_vkr.alloccb, &view));
        ASSERT(view);
        chain->views[i] = view;
    }

    {
        const VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
        chain->depthFormat = depthFormat;
        const VkImageCreateInfo depthInfo =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = depthFormat,
            .extent.width = chain->width,
            .extent.height = chain->height,
            .extent.depth = 1,
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };
        vkrImage_New(
            &chain->depthImage,
            &depthInfo,
            vkrMemUsage_GpuOnly,
            PIM_FILELINE);
        ASSERT(chain->depthImage.handle);
        const VkImageViewCreateInfo viewInfo =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = chain->depthImage.handle,
            .format = depthFormat,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
            .subresourceRange.levelCount = 1,
            .subresourceRange.layerCount = 1,
        };
        VkImageView view = NULL;
        VkCheck(vkCreateImageView(dev, &viewInfo, g_vkr.alloccb, &view));
        ASSERT(view);
        chain->depthView = view;
    }

    for (i32 i = 0; i < kFramesInFlight; ++i)
    {
        chain->syncFences[i] = vkrFence_New(true);
        chain->availableSemas[i] = vkrSemaphore_New();
        chain->renderedSemas[i] = vkrSemaphore_New();
    }

    return true;
}

void vkrSwapchain_Del(vkrSwapchain* chain)
{
    if (chain && chain->handle)
    {
        VkDevice dev = g_vkr.dev;
        ASSERT(dev);

        vkDeviceWaitIdle(dev);

        for (i32 i = 0; i < kFramesInFlight; ++i)
        {
            vkrFence_Del(chain->syncFences[i]);
            vkrSemaphore_Del(chain->availableSemas[i]);
            vkrSemaphore_Del(chain->renderedSemas[i]);
        }

        vkDestroyImageView(dev, chain->depthView, g_vkr.alloccb);
        vkrImage_Del(&chain->depthImage);

        {
            const i32 len = chain->length;
            for (i32 i = 0; i < len; ++i)
            {
                VkImageView view = chain->views[i];
                VkFramebuffer buffer = chain->buffers[i];
                if (view)
                {
                    vkDestroyImageView(dev, view, g_vkr.alloccb);
                }
                if (buffer)
                {
                    vkDestroyFramebuffer(dev, buffer, g_vkr.alloccb);
                }
            }
        }

        vkDestroySwapchainKHR(dev, chain->handle, g_vkr.alloccb);

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
        VkImageView attachments[] =
        {
            chain->views[i],
            chain->depthView,
        };
        ASSERT(attachments[0]);
        ASSERT(attachments[1]);
        VkFramebuffer buffer = chain->buffers[i];
        if (buffer)
        {
            vkDestroyFramebuffer(g_vkr.dev, buffer, g_vkr.alloccb);
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
        VkCheck(vkCreateFramebuffer(g_vkr.dev, &bufferInfo, g_vkr.alloccb, &buffer));
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
u32 vkrSwapchain_AcquireSync(vkrSwapchain* chain)
{
    ProfileBegin(pm_acquiresync);

    ASSERT(chain);
    ASSERT(chain->handle);

    const u32 syncIndex = (chain->syncIndex + 1) % kFramesInFlight;
    VkFence fence = chain->syncFences[syncIndex];
    if (fence)
    {
        // acquire resources associated with this frame in flight
        vkrFence_Wait(fence);
    }
    chain->syncIndex = syncIndex;

    ProfileEnd(pm_acquiresync);

    return syncIndex;
}

ProfileMark(pm_acquireimg, vkrSwapchain_AcquireImage)
u32 vkrSwapchain_AcquireImage(vkrSwapchain* chain)
{
    ProfileBegin(pm_acquireimg);

    ASSERT(chain);
    ASSERT(chain->handle);
    ASSERT(chain->length > 0);

    const u32 syncIndex = chain->syncIndex;
    VkSemaphore beginSema = chain->availableSemas[syncIndex];
    ASSERT(beginSema);

    u32 imageIndex = 0;
    const u64 timeout = -1;
    vkAcquireNextImageKHR(
        g_vkr.dev,
        chain->handle,
        timeout,
        beginSema,
        NULL,
        &imageIndex);

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
    ASSERT(syncIndex < kFramesInFlight);
    chain->imageFences[imageIndex] = chain->syncFences[syncIndex];
    chain->imageIndex = imageIndex;

    ProfileEnd(pm_acquireimg);

    return imageIndex;
}

ProfileMark(pm_present, vkrSwapchain_Present)
void vkrSwapchain_Present(
    vkrSwapchain* chain,
    VkQueue submitQueue,
    VkCommandBuffer cmdbuf)
{
    ASSERT(chain);
    ASSERT(chain->handle);
    ASSERT(cmdbuf);

    const u32 syncIndex = chain->syncIndex;
    VkFence signalFence = chain->syncFences[syncIndex];
    vkrFence_Reset(signalFence);
    VkSemaphore waitSema = chain->availableSemas[syncIndex];
    const VkPipelineStageFlags waitMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSemaphore signalSema = chain->renderedSemas[syncIndex];
    vkrCmdSubmit(
        submitQueue,
        cmdbuf,
        signalFence,
        waitSema,
        waitMask,
        signalSema);

    ProfileBegin(pm_present);

    VkQueue presentQueue = g_vkr.queues[vkrQueueId_Pres].handle;
    ASSERT(presentQueue);
    const VkSwapchainKHR swapchains[] =
    {
        chain->handle,
    };
    const uint32_t imageIndices[] =
    {
        chain->imageIndex,
    };
    const VkPresentInfoKHR presentInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &signalSema,
        .swapchainCount = NELEM(swapchains),
        .pSwapchains = swapchains,
        .pImageIndices = imageIndices,
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
