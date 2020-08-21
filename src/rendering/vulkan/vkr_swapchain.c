#include "rendering/vulkan/vkr_swapchain.h"
#include "rendering/vulkan/vkr_display.h"
#include "rendering/vulkan/vkr_queue.h"
#include "allocator/allocator.h"
#include "common/console.h"
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

    u32 maxImages = i1_min(kMaxSwapchainLength, sup.caps.maxImageCount);
    u32 imgCount = i1_clamp(2, sup.caps.minImageCount, maxImages);

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
    chain->length = imgCount;
    ASSERT(imgCount <= kMaxSwapchainLength);
    VkCheck(vkGetSwapchainImagesKHR(dev, handle, &imgCount, chain->images));

    for (u32 i = 0; i < imgCount; ++i)
    {
        ASSERT(chain->images[i]);
        const VkImageViewCreateInfo viewInfo =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = chain->images[i],
            .format = chain->format,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.levelCount = 1,
            .subresourceRange.layerCount = 1,
        };
        VkCheck(vkCreateImageView(dev, &viewInfo, NULL, chain->views + i));
        ASSERT(chain->views[i]);
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

            const i32 len = chain->length;
            VkImageView* views = chain->views;
            VkFramebuffer* buffers = chain->buffers;
            for (i32 i = 0; i < len; ++i)
            {
                VkImageView view = views[i];
                views[i] = NULL;
                if (view)
                {
                    vkDestroyImageView(dev, view, NULL);
                }
                VkFramebuffer buffer = buffers[i];
                buffers[i] = NULL;
                if (buffer)
                {
                    vkDestroyFramebuffer(dev, buffer, NULL);
                }
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
    for (i32 i = 0; i < len; ++i)
    {
        ASSERT(chain->views[i]);
        const VkImageView attachments[] =
        {
            chain->views[i],
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
        if (chain->buffers[i])
        {
            vkDestroyFramebuffer(g_vkr.dev, chain->buffers[i], NULL);
            chain->buffers[i] = NULL;
        }
        VkFramebuffer buffer = NULL;
        VkCheck(vkCreateFramebuffer(g_vkr.dev, &bufferInfo, NULL, &buffer));
        ASSERT(buffer);
        chain->buffers[i] = buffer;
    }
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
