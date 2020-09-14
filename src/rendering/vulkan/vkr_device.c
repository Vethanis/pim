#include "rendering/vulkan/vkr_device.h"
#include "rendering/vulkan/vkr_instance.h"
#include "rendering/vulkan/vkr_queue.h"
#include "rendering/vulkan/vkr_swapchain.h"
#include "allocator/allocator.h"
#include "common/console.h"
#include "common/stringutil.h"
#include <GLFW/glfw3.h>
#include <string.h>

bool vkrDevice_Init(vkr_t* vkr)
{
    ASSERT(vkr);

    vkr->phdev = vkrSelectPhysicalDevice(
        &vkr->display,
        &vkr->phdevProps,
        &vkr->phdevFeats);
    ASSERT(vkr->phdev);
    if (!vkr->phdev)
    {
        return false;
    }

    vkrListDevExtensions(vkr->phdev);

    vkr->dev = vkrCreateDevice(
        &vkr->display,
        vkrGetDevExtensions(vkr->phdev),
        vkrGetLayers());
    ASSERT(vkr->dev);
    if (!vkr->dev)
    {
        return false;
    }

    volkLoadDevice(vkr->dev);

    vkrCreateQueues(vkr);

    return true;
}

void vkrDevice_Shutdown(vkr_t* vkr)
{
    if (vkr)
    {
        vkrDestroyQueues(vkr);
        if (vkr->dev)
        {
            vkDestroyDevice(vkr->dev, NULL);
            vkr->dev = NULL;
        }
        vkr->phdev = NULL;
    }
}

void vkrDevice_WaitIdle(void)
{
    ASSERT(g_vkr.dev);
    VkCheck(vkDeviceWaitIdle(g_vkr.dev));
}

// ----------------------------------------------------------------------------

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

void vkrListDevExtensions(VkPhysicalDevice phdev)
{
    ASSERT(phdev);

    u32 count = 0;
    VkExtensionProperties* props = vkrEnumDevExtensions(phdev, &count);
    con_logf(LogSev_Info, "vkr", "%d available device extensions", count);
    for (u32 i = 0; i < count; ++i)
    {
        con_logf(LogSev_Info, "vkr", props[i].extensionName);
    }
}

static const char* const kRequiredDevExtensions[] =
{
    // https://www.khronos.org/registry/vulkan/specs/1.0-wsi_extensions/html/vkspec.html#VK_KHR_swapchain
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_EXT_MEMORY_BUDGET_EXTENSION_NAME,
    VK_NV_RAY_TRACING_EXTENSION_NAME,
    VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME,
    VK_KHR_16BIT_STORAGE_EXTENSION_NAME,
    VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
};

static const char* const kDesiredDevExtensions[] =
{
    VK_EXT_CONDITIONAL_RENDERING_EXTENSION_NAME,
    VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME,
    VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME,
    VK_KHR_BIND_MEMORY_2_EXTENSION_NAME,
    VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
    VK_KHR_SPIRV_1_4_EXTENSION_NAME,
    VK_NV_MESH_SHADER_EXTENSION_NAME,
    VK_EXT_HDR_METADATA_EXTENSION_NAME,
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
            con_logf(LogSev_Error, "vkr", "Failed to load required device extension '%s'", name);
        }
    }

    for (i32 i = 0; i < NELEM(kDesiredDevExtensions); ++i)
    {
        const char* name = kDesiredDevExtensions[i];
        if (!vkrTryAddExtension(&list, props, count, name))
        {
            con_logf(LogSev_Warning, "vkr", "Failed to load desired device extension '%s'", name);
        }
    }

    return list;
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
    const vkrDisplay* display,
    VkPhysicalDeviceProperties* propsOut,
    VkPhysicalDeviceFeatures* featuresOut)
{
    VkInstance inst = g_vkr.inst;
    ASSERT(inst);
    ASSERT(display);
    VkSurfaceKHR surface = display->surface;
    ASSERT(surface);

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
        vkrQueueSupport support = vkrQueryQueueSupport(phdev, surface);

        bool supportsAll = true;
        for (i32 id = 0; id < vkrQueueId_COUNT; ++id)
        {
            i32 family = support.family[id];
            supportsAll &= (family >= 0);
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
    const vkrDisplay* display,
    strlist_t extensions,
    strlist_t layers)
{
    VkPhysicalDevice phdev = g_vkr.phdev;
    ASSERT(phdev);
    ASSERT(display);
    VkSurfaceKHR surface = display->surface;
    ASSERT(surface);

    const vkrQueueSupport support = vkrQueryQueueSupport(phdev, surface);

    i32 familyCount = 0;
    i32 families[vkrQueueId_COUNT] = { 0 };
    i32 counts[vkrQueueId_COUNT] = { 0 };
    for (i32 id = 0; id < vkrQueueId_COUNT; ++id)
    {
        i32 family = support.family[id];
        ASSERT(family >= 0);
        i32 location = -1;
        for (i32 j = 0; j < familyCount; ++j)
        {
            if (families[j] == family)
            {
                location = j;
                break;
            }
        }
        if (location == -1)
        {
            location = familyCount;
            ++familyCount;
            families[location] = family;
        }
        counts[location] += 1;
    }

    const float priorities[] =
    {
        1.0f, 1.0f, 1.0f, 1.0f,
    };
    VkDeviceQueueCreateInfo queueInfos[vkrQueueId_COUNT] = { 0 };
    for (i32 i = 0; i < familyCount; ++i)
    {
        queueInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfos[i].queueFamilyIndex = families[i];
        queueInfos[i].queueCount = counts[i];
        queueInfos[i].pQueuePriorities = priorities;
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
        .independentBlend = availableFeatures->independentBlend,

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
        .queueCreateInfoCount = familyCount,
        .pQueueCreateInfos = queueInfos,
        .pEnabledFeatures = &phDevFeatures,
        .enabledLayerCount = layers.count,
        .ppEnabledLayerNames = layers.ptr,
        .enabledExtensionCount = extensions.count,
        .ppEnabledExtensionNames = extensions.ptr,
    };

    VkDevice device = NULL;
    VkCheck(vkCreateDevice(g_vkr.phdev, &devInfo, NULL, &device));
    ASSERT(device);

    strlist_del(&extensions);
    strlist_del(&layers);

    return device;
}
