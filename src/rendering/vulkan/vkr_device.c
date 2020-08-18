#include "rendering/vulkan/vkr_device.h"
#include "rendering/vulkan/vkr_swapchain.h"
#include "allocator/allocator.h"
#include "common/console.h"
#include "common/stringutil.h"
#include <GLFW/glfw3.h>
#include <string.h>

VkExtensionProperties* vkrEnumInstExtensions(u32* countOut)
{
    ASSERT(countOut);
    u32 count = 0;
    VkExtensionProperties* props = NULL;
    VkCheck(vkEnumerateInstanceExtensionProperties(NULL, &count, NULL));
    TempReserve(props, count);
    VkCheck(vkEnumerateInstanceExtensionProperties(NULL, &count, props));
    *countOut = count;
    return props;
}

VkExtensionProperties* vkrEnumDevExtensions(
    VkPhysicalDevice phdev,
    u32* countOut)
{
    ASSERT(phdev);
    ASSERT(countOut);
    u32 count = 0;
    VkExtensionProperties* props = NULL;
    VkCheck(vkEnumerateDeviceExtensionProperties(phdev, NULL, &count, NULL));
    TempReserve(props, count);
    VkCheck(vkEnumerateDeviceExtensionProperties(phdev, NULL, &count, props));
    *countOut = count;
    return props;
}

i32 vkrFindExtension(
    const VkExtensionProperties* props,
    u32 count,
    const char* extName)
{
    ASSERT(props || !count);
    ASSERT(extName);
    for (u32 i = 0; i < count; ++i)
    {
        if (StrCmp(ARGS(props[i].extensionName), extName) == 0)
        {
            return (i32)i;
        }
    }
    return -1;
}

bool vkrTryAddExtension(
    strlist_t* list,
    const VkExtensionProperties* props,
    u32 propCount,
    const char* extName)
{
    ASSERT(list);
    if (vkrFindExtension(props, propCount, extName) != -1)
    {
        strlist_add(list, extName);
        return true;
    }
    return false;
}

void vkrListInstExtensions(void)
{
    u32 count = 0;
    VkExtensionProperties* props = vkrEnumInstExtensions(&count);
    con_logf(LogSev_Info, "Vk", "%d available instance extensions", count);
    for (u32 i = 0; i < count; ++i)
    {
        con_logf(LogSev_Info, "Vk", props[i].extensionName);
    }
}

void vkrListDevExtensions(VkPhysicalDevice phdev)
{
    ASSERT(phdev);

    u32 count = 0;
    VkExtensionProperties* props = vkrEnumDevExtensions(phdev, &count);
    con_logf(LogSev_Info, "Vk", "%d available device extensions", count);
    for (u32 i = 0; i < count; ++i)
    {
        con_logf(LogSev_Info, "Vk", props[i].extensionName);
    }
}

strlist_t vkrGetLayers(void)
{
    strlist_t list;
    strlist_new(&list, EAlloc_Temp);
#ifdef _DEBUG
    strlist_add(&list, "VK_LAYER_KHRONOS_validation");
#endif // _DEBUG
    return list;
}

strlist_t vkrGetInstExtensions(void)
{
    strlist_t list;
    strlist_new(&list, EAlloc_Temp);

    u32 count = 0;
    VkExtensionProperties* props = vkrEnumInstExtensions(&count);

    u32 glfwCount = 0;
    const char** glfwList = glfwGetRequiredInstanceExtensions(&glfwCount);
    for (u32 i = 0; i < glfwCount; ++i)
    {
        vkrTryAddExtension(&list, props, count, glfwList[i]);
    }

#ifdef _DEBUG
    // https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VK_EXT_debug_utils.html
    vkrTryAddExtension(&list, props, count, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif // _DEBUG

    return list;
}

static const char* const kRequiredDevExtensions[] =
{
    // https://www.khronos.org/registry/vulkan/specs/1.0-wsi_extensions/html/vkspec.html#VK_KHR_swapchain
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

static const char* const kDesiredDevExtensions[] =
{
    // https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#VK_KHR_performance_query
    VK_KHR_PERFORMANCE_QUERY_EXTENSION_NAME,
    // https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#VK_EXT_conditional_rendering
    VK_EXT_CONDITIONAL_RENDERING_EXTENSION_NAME,
};

strlist_t vkrGetDevExtensions(VkPhysicalDevice phdev)
{
    ASSERT(g_vkr.phdev);

    u32 count = 0;
    VkExtensionProperties* props = vkrEnumDevExtensions(phdev, &count);

    strlist_t list;
    strlist_new(&list, EAlloc_Temp);

    for (i32 i = 0; i < NELEM(kRequiredDevExtensions); ++i)
    {
        const char* name = kRequiredDevExtensions[i];
        if (!vkrTryAddExtension(&list, props, count, name))
        {
            con_logf(LogSev_Error, "Vk", "Failed to load required extension '%s'", name);
        }
    }

    for (i32 i = 0; i < NELEM(kDesiredDevExtensions); ++i)
    {
        const char* name = kDesiredDevExtensions[i];
        if (!vkrTryAddExtension(&list, props, count, name))
        {
            con_logf(LogSev_Warning, "Vk", "Failed to load desired extension '%s'", name);
        }
    }

    return list;
}

VkInstance vkrCreateInstance(strlist_t extensions, strlist_t layers)
{
    const VkApplicationInfo appInfo =
    {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pApplicationName = "pimquake",
        .applicationVersion = 1,
        .pEngineName = "pim",
        .engineVersion = 1,
        .apiVersion = VK_API_VERSION_1_2,
    };

    const VkInstanceCreateInfo instInfo =
    {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0x0,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = layers.count,
        .ppEnabledLayerNames = layers.ptr,
        .enabledExtensionCount = extensions.count,
        .ppEnabledExtensionNames = extensions.ptr,
    };

    VkInstance inst = NULL;
    VkCheck(vkCreateInstance(&instInfo, NULL, &inst));

    strlist_del(&extensions);
    strlist_del(&layers);

    return inst;
}

u32 vkrEnumPhysicalDevices(
    VkInstance inst,
    VkPhysicalDevice** pDevices,
    VkPhysicalDeviceFeatures** pFeatures,
    VkPhysicalDeviceProperties** pProps)
{
    ASSERT(inst);

    u32 count = 0;
    VkCheck(vkEnumeratePhysicalDevices(inst, &count, NULL));

    if (pDevices)
    {
        VkPhysicalDevice* devices = tmp_calloc(sizeof(devices[0]) * count);
        VkCheck(vkEnumeratePhysicalDevices(inst, &count, devices));
        *pDevices = devices;

        if (pFeatures)
        {
            VkPhysicalDeviceFeatures* features = tmp_calloc(sizeof(features[0]) * count);
            for (u32 i = 0; i < count; ++i)
            {
                ASSERT(devices[i]);
                vkGetPhysicalDeviceFeatures(devices[i], features + i);
            }
            *pFeatures = features;
        }

        if (pProps)
        {
            VkPhysicalDeviceProperties* props = tmp_calloc(sizeof(props[0]) * count);
            for (u32 i = 0; i < count; ++i)
            {
                ASSERT(devices[i]);
                vkGetPhysicalDeviceProperties(devices[i], props + i);
            }
            *pProps = props;
        }
    }

    return count;
}

VkPhysicalDevice vkrSelectPhysicalDevice(
    VkPhysicalDeviceProperties* propsOut,
    VkPhysicalDeviceFeatures* featuresOut)
{
    VkInstance inst = g_vkr.inst;
    VkSurfaceKHR surf = g_vkrdisp.surf;
    ASSERT(inst);
    ASSERT(surf);

    if (propsOut)
    {
        memset(propsOut, 0, sizeof(*propsOut));
    }
    if (featuresOut)
    {
        memset(featuresOut, 0, sizeof(*featuresOut));
    }

    VkPhysicalDevice* devices = NULL;
    VkPhysicalDeviceFeatures* feats = NULL;
    VkPhysicalDeviceProperties* props = NULL;
    u32 count = vkrEnumPhysicalDevices(inst, &devices, &feats, &props);
    i32* desiredExtCounts = tmp_calloc(sizeof(desiredExtCounts[0]) * count);
    bool* hasRequiredExts = tmp_calloc(sizeof(hasRequiredExts[0]) * count);
    bool* hasQueueSupport = tmp_calloc(sizeof(hasQueueSupport[0]) * count);

    for (u32 i = 0; i < count; ++i)
    {
        VkPhysicalDevice phdev = devices[i];
        u32 extCount = 0;
        VkExtensionProperties* exts = vkrEnumDevExtensions(phdev, &extCount);

        bool hasRequired = true;
        for (i32 j = 0; j < NELEM(kRequiredDevExtensions); ++j)
        {
            if (vkrFindExtension(exts, extCount, kRequiredDevExtensions[j]) < 0)
            {
                hasRequired = false;
                break;
            }
        }
        hasRequiredExts[i] = hasRequired;

        i32 desiredCount = 0;
        for (i32 j = 0; j < NELEM(kDesiredDevExtensions); ++j)
        {
            if (vkrFindExtension(exts, extCount, kDesiredDevExtensions[j]) >= 0)
            {
                ++desiredCount;
            }
        }
        desiredExtCounts[i] = desiredCount;
    }

    for (u32 i = 0; i < count; ++i)
    {
        VkPhysicalDevice phdev = devices[i];
        vkrQueueSupport qsup = vkrQueryQueueSupport(phdev, surf);

        bool supportsAll = true;
        for (i32 j = 0; j < vkrQueueId_COUNT; ++j)
        {
            if (qsup.family[j] < 0)
            {
                supportsAll = false;
                break;
            }
        }

        hasQueueSupport[i] = supportsAll;
    }

    i32 c = -1;
    for (u32 i = 0; i < count; ++i)
    {
        VkPhysicalDevice phdev = devices[i];

        if (!hasRequiredExts[i])
        {
            continue;
        }

        if (!hasQueueSupport[i])
        {
            continue;
        }

        if (c == -1)
        {
            c = i;
            continue;
        }

        if (desiredExtCounts[i] > desiredExtCounts[c])
        {
            c = i;
            continue;
        }

        i32 limCmp = memcmp(&props[i].limits, &props[c].limits, sizeof(props[0].limits));
        i32 featCmp = memcmp(&feats[i], &feats[c], sizeof(feats[0]));

        if ((limCmp < 0) || (featCmp < 0))
        {
            // worse limits or features, skip i
            continue;
        }
        if ((limCmp > 0) || (featCmp > 0))
        {
            // better limits or features, choose i
            c = i;
            continue;
        }
    }

    if (c != -1)
    {
        if (propsOut)
        {
            *propsOut = props[c];
        }
        if (featuresOut)
        {
            *featuresOut = feats[c];
        }
        return devices[c];
    }

    return NULL;
}

VkDevice vkrCreateDevice(
    strlist_t extensions,
    strlist_t layers,
    VkQueue* queuesOut,
    VkQueueFamilyProperties* propsOut)
{
    VkPhysicalDevice phdev = g_vkr.phdev;
    VkSurfaceKHR surf = g_vkrdisp.surf;
    ASSERT(phdev);
    ASSERT(surf);

    const vkrQueueSupport support = vkrQueryQueueSupport(phdev, surf);

    i32 famCount = 0;
    i32 fams[vkrQueueId_COUNT];

    for (i32 i = 0; i < vkrQueueId_COUNT; ++i)
    {
        i32 iFam = support.family[i];
        ASSERT(iFam >= 0);
        bool exists = false;
        for (i32 j = 0; j < famCount; ++j)
        {
            exists |= fams[j] == iFam;
        }
        if (!exists)
        {
            fams[famCount++] = iFam;
        }
    }

    const float priority = 1.0f;
    VkDeviceQueueCreateInfo queueInfos[vkrQueueId_COUNT] = { 0 };
    for (i32 i = 0; i < famCount; ++i)
    {
        queueInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfos[i].queueFamilyIndex = fams[i];
        queueInfos[i].queueCount = 1;
        queueInfos[i].pQueuePriorities = &priority;
    }

    const VkPhysicalDeviceFeatures* availableFeatures = &g_vkr.phdevFeats;

    // features we intend to use
    const VkPhysicalDeviceFeatures phDevFeatures =
    {
        // basic rasterizer features
        .fillModeNonSolid = availableFeatures->fillModeNonSolid,
        .wideLines = availableFeatures->wideLines,
        .largePoints = availableFeatures->largePoints,
        .depthClamp = availableFeatures->depthClamp,
        .depthBounds = availableFeatures->depthBounds,

        // anisotropic sampling
        .samplerAnisotropy = availableFeatures->samplerAnisotropy,

        // block compression
        .textureCompressionETC2 = availableFeatures->textureCompressionETC2,
        .textureCompressionASTC_LDR = availableFeatures->textureCompressionASTC_LDR,
        .textureCompressionBC = availableFeatures->textureCompressionBC,

        // throughput statistics
        .pipelineStatisticsQuery = availableFeatures->pipelineStatisticsQuery,

        // indexing resources in shaders
        .shaderUniformBufferArrayDynamicIndexing = availableFeatures->shaderUniformBufferArrayDynamicIndexing,
        .shaderStorageBufferArrayDynamicIndexing = availableFeatures->shaderStorageBufferArrayDynamicIndexing,
        .shaderSampledImageArrayDynamicIndexing = availableFeatures->shaderSampledImageArrayDynamicIndexing,
        .shaderStorageImageArrayDynamicIndexing = availableFeatures->shaderStorageImageArrayDynamicIndexing,

        // cubemap arrays
        .imageCubeArray = availableFeatures->imageCubeArray,

        // indirect rendering
        .multiDrawIndirect = availableFeatures->multiDrawIndirect,
    };

    const VkDeviceCreateInfo devInfo =
    {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = famCount,
        .pQueueCreateInfos = queueInfos,
        .pEnabledFeatures = &phDevFeatures,
        .enabledLayerCount = layers.count,
        .ppEnabledLayerNames = layers.ptr,
        .enabledExtensionCount = extensions.count,
        .ppEnabledExtensionNames = extensions.ptr,
    };

    VkDevice device = NULL;
    VkCheck(vkCreateDevice(g_vkr.phdev, &devInfo, NULL, &device));

    strlist_del(&extensions);
    strlist_del(&layers);

    if (device)
    {
        for (i32 i = 0; i < vkrQueueId_COUNT; ++i)
        {
            i32 fam = support.family[i];
            if (queuesOut)
            {
                vkGetDeviceQueue(device, fam, 0, queuesOut + i);
            }
            if (propsOut)
            {
                propsOut[i] = support.props[fam];
            }
        }
    }

    return device;
}
