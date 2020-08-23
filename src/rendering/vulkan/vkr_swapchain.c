#include "rendering/vulkan/vkr_swapchain.h"
#include "rendering/vulkan/vkr_display.h"
#include "rendering/vulkan/vkr_queue.h"
#include "rendering/vulkan/vkr_sync.h"
#include "rendering/vulkan/vkr_device.h"
#include "allocator/allocator.h"
#include "common/console.h"
#include "common/profiler.h"
#include "math/scalar.h"
#include <GLFW/glfw3.h>
#include <string.h>

vkrSwapchain* vkrSwapchain_New(vkrDisplay* display, vkrSwapchain* prev)
{
    ASSERT(display);

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

    u32 imgCount = i1_clamp(2, sup.caps.minImageCount, sup.caps.maxImageCount);

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
    VkCheck(vkCreateSwapchainKHR(dev, &swapInfo, NULL, &handle));
    ASSERT(handle);
    if (!handle)
    {
        con_logf(LogSev_Error, "Vk", "Failed to create swapchain");
        return NULL;
    }

    vkrSwapchain* chain = perm_calloc(sizeof(*chain));
    chain->refcount = 1;
    chain->handle = handle;
    chain->mode = mode;
    chain->format = format.format;
    chain->colorSpace = format.colorSpace;
    chain->width = ext.width;
    chain->height = ext.height;

    vkrDisplay_Retain(display);
    chain->display = display;

    VkCheck(vkGetSwapchainImagesKHR(dev, handle, &imgCount, NULL));
    VkImage* images = tmp_calloc(sizeof(images[0]) * imgCount);
    VkCheck(vkGetSwapchainImagesKHR(dev, handle, &imgCount, images));

    vkrSwapFrame* frames = perm_calloc(sizeof(frames[0]) * imgCount);
    for (u32 i = 0; i < imgCount; ++i)
    {
        VkImage image = images[i];
        ASSERT(image);

        const VkImageViewCreateInfo viewInfo =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = image,
            .format = chain->format,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.levelCount = 1,
            .subresourceRange.layerCount = 1,
        };
        VkImageView view = NULL;
        VkCheck(vkCreateImageView(dev, &viewInfo, NULL, &view));
        ASSERT(view);

        frames[i].image = image;
        frames[i].view = view;
    }
    chain->length = imgCount;
    chain->frames = frames;

    for (i32 i = 0; i < kFramesInFlight; ++i)
    {
        chain->fences[i] = vkrCreateFence(true);
        chain->availableSemas[i] = vkrCreateSemaphore();
        chain->renderedSemas[i] = vkrCreateSemaphore();
    }

    return chain;
}

void vkrSwapchain_Retain(vkrSwapchain* chain)
{
    if (vkrAlive(chain))
    {
        chain->refcount += 1;
    }
}

void vkrSwapchain_Release(vkrSwapchain* chain)
{
    if (vkrAlive(chain))
    {
        chain->refcount -= 1;
        if (chain->refcount == 0)
        {
            VkDevice dev = g_vkr.dev;
            ASSERT(dev);
            vkDeviceWaitIdle(dev);
            for (i32 i = 0; i < kFramesInFlight; ++i)
            {
                vkrDestroyFence(chain->fences[i]);
                chain->fences[i] = NULL;
                vkrDestroySemaphore(chain->availableSemas[i]);
                chain->availableSemas[i] = NULL;
                vkrDestroySemaphore(chain->renderedSemas[i]);
                chain->renderedSemas[i] = NULL;
            }
            {
                const i32 len = chain->length;
                vkrSwapFrame* images = chain->frames;
                chain->length = 0;
                chain->frames = NULL;
                for (i32 i = 0; i < len; ++i)
                {
                    if (images[i].view)
                    {
                        vkDestroyImageView(dev, images[i].view, NULL);
                    }
                    if (images[i].buffer)
                    {
                        vkDestroyFramebuffer(dev, images[i].buffer, NULL);
                    }
                    images[i].view = NULL;
                    images[i].buffer = NULL;
                    images[i].image = NULL;
                }
                pim_free(images);
            }

            vkDestroySwapchainKHR(dev, chain->handle, NULL);

            vkrDisplay_Release(chain->display);

            memset(chain, 0, sizeof(*chain));
            pim_free(chain);
        }
    }
}

void vkrSwapchain_SetupBuffers(vkrSwapchain* chain, vkrRenderPass* presentPass)
{
    ASSERT(vkrAlive(chain));
    ASSERT(vkrAlive(presentPass));
    const i32 len = chain->length;
    vkrSwapFrame* images = chain->frames;
    for (i32 i = 0; i < len; ++i)
    {
        VkImageView view = images[i].view;
        VkFramebuffer buffer = images[i].buffer;
        if (buffer)
        {
            vkDestroyFramebuffer(g_vkr.dev, buffer, NULL);
            buffer = NULL;
        }

        ASSERT(view);
        const VkImageView attachments[] =
        {
            view,
        };
        const VkFramebufferCreateInfo bufferInfo =
        {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = presentPass->handle,
            .attachmentCount = NELEM(attachments),
            .pAttachments = attachments,
            .width = chain->width,
            .height = chain->height,
            .layers = 1,
        };
        VkCheck(vkCreateFramebuffer(g_vkr.dev, &bufferInfo, NULL, &buffer));
        ASSERT(buffer);

        images[i].buffer = buffer;
    }
}

vkrSwapchain* vkrSwapchain_Recreate(
    vkrDisplay* display,
    vkrSwapchain* prev,
    vkrRenderPass* presentPass)
{
    ASSERT(vkrAlive(display));
    ASSERT(vkrAlive(presentPass));
    vkrSwapchain* chain = NULL;
    vkrDevice_WaitIdle();
    if ((display->width > 0) && (display->height > 0))
    {
        chain = vkrSwapchain_New(display, prev);
        vkrSwapchain_SetupBuffers(chain, presentPass);
    }
    vkrSwapchain_Release(prev);
    return chain;
}

ProfileMark(pm_acquire, vkrSwapchain_Acquire)
u32 vkrSwapchain_Acquire(
    vkrSwapchain* chain,
    vkrSwapFrame* dstFrame)
{
    ProfileBegin(pm_acquire);

    ASSERT(vkrAlive(chain));
    ASSERT(chain->handle);
    ASSERT(dstFrame);

    const u32 syncIndex = chain->syncIndex;
    VkSemaphore beginSema = chain->availableSemas[syncIndex];
    ASSERT(beginSema);

    const u32 imageCount = chain->length;
    ASSERT(imageCount > 0);
    u32 imageIndex = 0;
    const u64 timeout = -1;
    vkAcquireNextImageKHR(
        g_vkr.dev,
        chain->handle,
        timeout,
        beginSema,
        NULL,
        &imageIndex);

    ASSERT(imageIndex < imageCount);
    vkrSwapFrame* frames = chain->frames;
    {
        VkFence imageFence = frames[imageIndex].fence;
        if (imageFence)
        {
            vkrWaitFence(imageFence);
        }
    }
    frames[imageIndex].fence = chain->fences[syncIndex];
    chain->imageIndex = imageIndex;

    *dstFrame = frames[imageIndex];

    ProfileEnd(pm_acquire);
    return syncIndex;
}

ProfileMark(pm_present, vkrSwapchain_Present)
ProfileMark(pm_submit, vkrSwapchain_Submit)
void vkrSwapchain_Present(
    vkrSwapchain* chain,
    vkrQueueId cmdQueue,
    VkCommandBuffer cmd)
{
ProfileBegin(pm_submit);

    ASSERT(vkrAlive(chain));
    ASSERT(chain->handle);
    ASSERT(cmd);
    VkQueue submitQueue = g_vkr.queues[cmdQueue].handle;
    VkQueue presentQueue = g_vkr.queues[vkrQueueId_Pres].handle;
    ASSERT(submitQueue);
    ASSERT(presentQueue);

    const u32 syncIndex = chain->syncIndex;
    VkFence signalFence = chain->fences[syncIndex];
    vkrResetFence(signalFence);
    const VkSemaphore waitSemaphores[] =
    {
        chain->availableSemas[syncIndex],
    };
    const VkPipelineStageFlags waitMasks[] =
    {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    };
    const VkSemaphore signalSemaphores[] =
    {
        chain->renderedSemas[syncIndex],
    };
    const VkSubmitInfo submitInfo =
    {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = NELEM(waitSemaphores),
        .pWaitSemaphores = waitSemaphores,
        .pWaitDstStageMask = waitMasks,
        .signalSemaphoreCount = NELEM(signalSemaphores),
        .pSignalSemaphores = signalSemaphores,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd,
    };
    VkCheck(vkQueueSubmit(submitQueue, 1, &submitInfo, signalFence));

ProfileEnd(pm_submit);
ProfileBegin(pm_present);

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
        .waitSemaphoreCount = NELEM(signalSemaphores),
        .pWaitSemaphores = signalSemaphores,
        .swapchainCount = NELEM(swapchains),
        .pSwapchains = swapchains,
        .pImageIndices = imageIndices,
    };
    VkCheck(vkQueuePresentKHR(presentQueue, &presentInfo));

    chain->syncIndex = (syncIndex + 1) % kFramesInFlight;

ProfileEnd(pm_present);
}

VkViewport vkrSwapchain_GetViewport(const vkrSwapchain* chain)
{
    ASSERT(vkrAlive(chain));
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
    ASSERT(vkrAlive(chain));
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
