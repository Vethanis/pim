#include "rendering/vulkan/vkr_swapchain.h"
#include "rendering/vulkan/vkr_display.h"
#include "rendering/vulkan/vkr_queue.h"
#include "rendering/vulkan/vkr_sync.h"
#include "rendering/vulkan/vkr_device.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_mem.h"
#include "rendering/vulkan/vkr_image.h"
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
    vkrSwapchain* prev)
{
    ASSERT(chain);
    memset(chain, 0, sizeof(*chain));

    vkrWindow *const window = &g_vkr.window;
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

    u32 imgCount = i1_clamp(R_DesiredSwapchainLen, sup.caps.minImageCount, i1_min(R_MaxSwapchainLen, sup.caps.maxImageCount));

    const u32 families[] =
    {
        qsup.family[vkrQueueId_Graphics],
        qsup.family[vkrQueueId_Present],
    };
    bool concurrent = families[0] != families[1];

    const u32 usage =
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

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
        .imageUsage = usage,
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
        Con_Logf(LogSev_Info, "vkr", "Present extent: %d x %d", ext.width, ext.height);
        Con_Logf(LogSev_Info, "vkr", "Present images: %d", imgCount);
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
    VkImage images[R_MaxSwapchainLen] = { 0 };
    {
        VkCheck(vkGetSwapchainImagesKHR(dev, handle, &imgCount, NULL));
        if (imgCount > R_MaxSwapchainLen)
        {
            ASSERT(false);
            vkDestroySwapchainKHR(dev, handle, NULL);
            memset(chain, 0, sizeof(*chain));
            return false;
        }
        chain->length = imgCount;
        VkCheck(vkGetSwapchainImagesKHR(dev, handle, &imgCount, images));
    }

    // import images
    for (u32 i = 0; i < imgCount; ++i)
    {
        const VkImageCreateInfo info =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = chain->colorFormat,
            .extent.width = chain->width,
            .extent.height = chain->height,
            .extent.depth = 1,
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = swapInfo.imageUsage,
            .sharingMode = swapInfo.imageSharingMode,
            .queueFamilyIndexCount = swapInfo.queueFamilyIndexCount,
            .pQueueFamilyIndices = swapInfo.pQueueFamilyIndices,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };
        vkrImage_Import(&chain->images[i], &info, images[i]);
    }

    // create synchronization objects
    for (i32 i = 0; i < R_ResourceSets; ++i)
    {
        chain->availableSemas[i] = vkrSemaphore_New();
        chain->renderedSemas[i] = vkrSemaphore_New();
    }

    return true;
}

void vkrSwapchain_Del(vkrSwapchain* chain)
{
    if (chain)
    {
        vkrDevice_WaitIdle();

        for (i32 i = 0; i < R_ResourceSets; ++i)
        {
            vkrSemaphore_Del(chain->availableSemas[i]);
            vkrSemaphore_Del(chain->renderedSemas[i]);
            chain->availableSemas[i] = NULL;
            chain->renderedSemas[i] = NULL;
        }

        const i32 len = chain->length;
        for (i32 i = 0; i < len; ++i)
        {
            vkrImage_Release(&chain->images[i]);
        }

        if (chain->handle)
        {
            vkDestroySwapchainKHR(g_vkr.dev, chain->handle, NULL);
            chain->handle = NULL;
        }

        memset(chain, 0, sizeof(*chain));
    }
}

bool vkrSwapchain_Recreate(void)
{
    vkrWindow* window = &g_vkr.window;
    ASSERT(window->handle);
    bool recreated = false;
    vkrSwapchain next = { 0 };
    if ((window->width > 0) && (window->height > 0))
    {
        recreated = vkrSwapchain_New(&next, &g_vkr.chain);
    }
    vkrSwapchain_Del(&g_vkr.chain);
    g_vkr.chain = next;
    return recreated;
}

ProfileMark(pm_acquiresync, vkrSwapchain_AcquireSync)
u32 vkrSwapchain_AcquireSync(void)
{
    ProfileBegin(pm_acquiresync);

    vkrSwapchain *const chain = &g_vkr.chain;
    ASSERT(chain->handle);

    u32 syncIndex = (chain->syncIndex + 1) % NELEM(chain->syncSubmits);
    vkrSubmitId submit = chain->syncSubmits[syncIndex];
    if (submit.valid)
    {
        vkrSubmit_Await(submit);
    }

    chain->syncIndex = syncIndex;

    ProfileEnd(pm_acquiresync);

    return syncIndex;
}

ProfileMark(pm_acquireimg, vkrSwapchain_AcquireImage)
u32 vkrSwapchain_AcquireImage(void)
{
    ProfileBegin(pm_acquireimg);

    vkrSwapchain *const chain = &g_vkr.chain;
    ASSERT(chain->handle);
    ASSERT(chain->length > 0);

    u32 syncIndex = chain->syncIndex;
    ASSERT(syncIndex < NELEM(chain->syncSubmits));

    u32 imageIndex = 0;
    const u64 timeout = -1;
    VkCheck(vkAcquireNextImageKHR(
        g_vkr.dev,
        chain->handle,
        timeout,
        chain->availableSemas[syncIndex],
        NULL,
        &imageIndex));

    // acquire resources associated with the frame in flight for this image
    // driver might not use same ring buffer index as the application
    ASSERT(imageIndex < (u32)chain->length);
    vkrSubmitId submit = chain->imageSubmits[imageIndex];
    if (submit.valid)
    {
        vkrSubmit_Await(submit);
    }
    chain->imageIndex = imageIndex;

    ProfileEnd(pm_acquireimg);

    return imageIndex;
}

ProfileMark(pm_submit, vkrSwapchain_Submit)
void vkrSwapchain_Submit(vkrCmdBuf* cmd)
{
    ProfileBegin(pm_submit);

    ASSERT(cmd && cmd->handle);
    ASSERT(cmd->fence);
    ASSERT(cmd->pres);
    vkrSwapchain *const chain = &g_vkr.chain;
    ASSERT(chain->handle);

    u32 imageIndex = chain->imageIndex;
    u32 syncIndex = chain->syncIndex;
    {
        vkrImage* backbuf = vkrGetBackBuffer();
        VkPipelineStageFlags prevUse = backbuf->state.stage;
        vkrImageState_PresentSrc(cmd, backbuf);

        vkrSubmitId submitId = vkrCmdSubmit(
            cmd,
            chain->availableSemas[syncIndex],
            prevUse,
            chain->renderedSemas[syncIndex]);

        chain->syncSubmits[syncIndex] = submitId;
        chain->imageSubmits[imageIndex] = submitId;
    }

    {
        const vkrQueue* queue = vkrGetQueue(vkrQueueId_Graphics);
        if (!queue->pres)
        {
            queue = vkrGetQueue(vkrQueueId_Present);
        }
        ASSERT(queue->handle);

        const VkPresentInfoKHR presentInfo =
        {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &chain->renderedSemas[syncIndex],
            .swapchainCount = 1,
            .pSwapchains = &chain->handle,
            .pImageIndices = &chain->imageIndex,
        };
        VkCheck(vkQueuePresentKHR(queue->handle, &presentInfo));
    }

    ProfileEnd(pm_submit);
}

VkViewport vkrSwapchain_GetViewport(void)
{
    ASSERT(g_vkr.chain.handle);
    VkViewport viewport =
    {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)g_vkr.chain.width,
        .height = (float)g_vkr.chain.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    return viewport;
}

VkRect2D vkrSwapchain_GetRect(void)
{
    ASSERT(g_vkr.chain.handle);
    VkRect2D rect =
    {
        .extent.width = g_vkr.chain.width,
        .extent.height = g_vkr.chain.height,
    };
    return rect;
}

float vkrSwapchain_GetAspect(void)
{
    ASSERT(g_vkr.chain.handle);
    return (float)g_vkr.chain.width / (float)g_vkr.chain.height;
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
