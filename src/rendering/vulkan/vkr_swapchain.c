#include "rendering/vulkan/vkr_swapchain.h"
#include "allocator/allocator.h"
#include "common/console.h"
#include <GLFW/glfw3.h>
#include <string.h>

VkSurfaceKHR vkrCreateSurface(VkInstance inst, GLFWwindow* win)
{
    ASSERT(inst);
    ASSERT(win);

    VkSurfaceKHR surf = NULL;
    VkCheck(glfwCreateWindowSurface(inst, win, NULL, &surf));

    return surf;
}

void vkrSelectFamily(i32* fam, i32 i, const VkQueueFamilyProperties* props)
{
    i32 c = *fam;
    if (c >= 0)
    {
        if (props[i].queueCount > props[c].queueCount)
        {
            c = i;
        }
    }
    else
    {
        c = i;
    }
    *fam = c;
}

VkQueueFamilyProperties* vkrEnumQueueFamilyProperties(
    VkPhysicalDevice phdev,
    u32* countOut)
{
    ASSERT(phdev);
    ASSERT(countOut);
    u32 count = 0;
    VkQueueFamilyProperties* props = NULL;
    vkGetPhysicalDeviceQueueFamilyProperties(phdev, &count, NULL);
    props = tmp_calloc(sizeof(props[0]) * count);
    vkGetPhysicalDeviceQueueFamilyProperties(phdev, &count, props);
    *countOut = count;
    return props;
}

vkrQueueSupport vkrQueryQueueSupport(VkPhysicalDevice phdev, VkSurfaceKHR surf)
{
    vkrQueueSupport support = { 0 };
    for (i32 i = 0; i < NELEM(support.family); ++i)
    {
        support.family[i] = -1;
    }

    ASSERT(phdev);
    ASSERT(surf);

    u32 count = 0;
    VkQueueFamilyProperties* props = vkrEnumQueueFamilyProperties(phdev, &count);
    support.count = count;
    support.props = props;

    for (u32 i = 0; i < count; ++i)
    {
        if (props[i].queueCount == 0)
        {
            continue;
        }
        u32 flags = props[i].queueFlags;

        if (flags & VK_QUEUE_GRAPHICS_BIT)
        {
            vkrSelectFamily(&support.family[vkrQueueId_Gfx], i, props);
        }
        if (flags & VK_QUEUE_COMPUTE_BIT)
        {
            vkrSelectFamily(&support.family[vkrQueueId_Comp], i, props);
        }
        if (flags & VK_QUEUE_TRANSFER_BIT)
        {
            vkrSelectFamily(&support.family[vkrQueueId_Xfer], i, props);
        }

        VkBool32 presentable = false;
        VkCheck(vkGetPhysicalDeviceSurfaceSupportKHR(phdev, i, surf, &presentable));
        if (presentable)
        {
            vkrSelectFamily(&support.family[vkrQueueId_Pres], i, props);
        }
    }

    return support;
}

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

i32 vkrSelectSwapFormat(const VkSurfaceFormatKHR* formats, u32 count)
{
    ASSERT(formats || !count);
    for (u32 p = 0; p < NELEM(kPreferredSurfaceFormats); ++p)
    {
        VkSurfaceFormatKHR pref = kPreferredSurfaceFormats[p];
        for (u32 i = 0; i < count; ++i)
        {
            VkSurfaceFormatKHR sf = formats[i];
            if (memcmp(&sf, &pref, sizeof(sf)) == 0)
            {
                return (i32)i;
            }
        }
    }
    return 0;
}

// pim prefers low latency over tear protection
static const VkPresentModeKHR kPreferredPresentModes[] =
{
    VK_PRESENT_MODE_MAILBOX_KHR,        // single-entry overwrote queue; good latency
    VK_PRESENT_MODE_IMMEDIATE_KHR,      // no queue; best latency, bad tearing
    VK_PRESENT_MODE_FIFO_RELAXED_KHR,   // multi-entry adaptive queue; bad latency
    VK_PRESENT_MODE_FIFO_KHR ,          // multi-entry queue; bad latency
};

i32 vkrSelectSwapMode(const VkPresentModeKHR* modes, u32 count)
{
    ASSERT(modes || !count);
    for (u32 p = 0; p < NELEM(kPreferredPresentModes); ++p)
    {
        VkPresentModeKHR pref = kPreferredPresentModes[p];
        for (u32 i = 0; i < count; ++i)
        {
            VkPresentModeKHR mode = modes[i];
            if (pref == mode)
            {
                return (i32)i;
            }
        }
    }
    return 0;
}

VkExtent2D vkrSelectSwapExtent(const VkSurfaceCapabilitiesKHR* caps, i32 width, i32 height)
{
    if (caps->currentExtent.width != ~0u)
    {
        return caps->currentExtent;
    }
    VkExtent2D ext = { width, height };
    VkExtent2D minExt = caps->minImageExtent;
    VkExtent2D maxExt = caps->maxImageExtent;
    ext.width = ext.width > minExt.width ? ext.width : minExt.width;
    ext.height = ext.height > minExt.height ? ext.height : minExt.height;
    ext.width = ext.width < maxExt.width ? ext.width : maxExt.width;
    ext.height = ext.height < maxExt.height ? ext.height : maxExt.height;
    return ext;
}

void vkrCreateSwapchain(vkrDisplay* display, VkSwapchainKHR prev)
{
    VkPhysicalDevice phdev = g_vkr.phdev;
    VkDevice dev = g_vkr.dev;
    VkSurfaceKHR surf = display->surf;
    i32 width = display->width;
    i32 height = display->height;
    ASSERT(phdev);
    ASSERT(dev);
    ASSERT(surf);

    vkrSwapchainSupport sup = vkrQuerySwapchainSupport(phdev, surf);
    vkrQueueSupport qsup = vkrQueryQueueSupport(phdev, surf);

    i32 iFormat = vkrSelectSwapFormat(sup.formats, sup.formatCount);
    i32 iMode = vkrSelectSwapMode(sup.modes, sup.modeCount);
    VkExtent2D ext = vkrSelectSwapExtent(&sup.caps, width, height);

    // just the minimum, to minimize latency
    u32 imgCount = sup.caps.minImageCount;

    const u32 families[] =
    {
        qsup.family[vkrQueueId_Gfx],
        qsup.family[vkrQueueId_Pres],
    };
    bool concurrent = families[0] != families[1];

    const VkSwapchainCreateInfoKHR swapInfo =
    {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surf,
        .presentMode = sup.modes[iMode],
        .minImageCount = imgCount,
        .imageFormat = sup.formats[iFormat].format,
        .imageColorSpace = sup.formats[iFormat].colorSpace,
        .imageExtent = ext,
        .imageArrayLayers = 1,
        // use transfer_dst_bit if rendering offscreen
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = concurrent ?
            VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = concurrent ? 2 : 0,
        .pQueueFamilyIndices = concurrent ? families : NULL,
        .preTransform = sup.caps.currentTransform,
        // no compositing with window manager / desktop background
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        // don't render pixels behind other windows
        .clipped = VK_TRUE,
        // prev swapchain, if re-creating
        .oldSwapchain = prev,
    };

    VkSwapchainKHR swap = NULL;
    VkCheck(vkCreateSwapchainKHR(dev, &swapInfo, NULL, &swap));
    ASSERT(swap);
    if (!swap)
    {
        con_logf(LogSev_Error, "Vk", "Failed to create swapchain");
        return;
    }

    display->swap = swap;
    display->format = sup.formats[iFormat].format;
    display->colorSpace = sup.formats[iFormat].colorSpace;
    display->width = ext.width;
    display->height = ext.height;

    VkCheck(vkGetSwapchainImagesKHR(dev, swap, &imgCount, NULL));
    PermReserve(display->images, imgCount);
    PermReserve(display->views, imgCount);
    VkCheck(vkGetSwapchainImagesKHR(dev, swap, &imgCount, display->images));
    display->imgCount = imgCount;

    for (u32 i = 0; i < imgCount; ++i)
    {
        const VkImageViewCreateInfo viewInfo =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = display->images[i],
            .format = display->format,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.levelCount = 1,
            .subresourceRange.layerCount = 1,
        };
        display->views[i] = NULL;
        VkCheck(vkCreateImageView(dev, &viewInfo, NULL, display->views + i));
        ASSERT(display->views[i]);
    }
}
