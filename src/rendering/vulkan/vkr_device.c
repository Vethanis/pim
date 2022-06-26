#include "rendering/vulkan/vkr_device.h"
#include "rendering/vulkan/vkr_instance.h"
#include "rendering/vulkan/vkr_queue.h"
#include "rendering/vulkan/vkr_swapchain.h"
#include "rendering/vulkan/vkr_extension.h"
#include "allocator/allocator.h"
#include "common/console.h"
#include "common/stringutil.h"
#include "math/scalar.h"
#include <string.h>

// ----------------------------------------------------------------------------

bool vkrDevice_Init(vkrSys* vkr)
{
    ASSERT(vkr);

    vkr->phdev = vkrSelectPhysicalDevice(
        &vkr->window,
        &g_vkrProps,
        &g_vkrFeats,
        &g_vkrDevExts);
    ASSERT(vkr->phdev);
    if (!vkr->phdev)
    {
        return false;
    }

    vkrListDevExtensions(vkr->phdev);

    vkr->dev = vkrCreateDevice(
        &vkr->window,
        vkrDevExts_ToList(&g_vkrDevExts),
        vkrGetLayers(&g_vkrLayers));
    ASSERT(vkr->dev);
    if (!vkr->dev)
    {
        return false;
    }

    volkLoadDevice(vkr->dev);

    vkrCreateQueues();

    return true;
}

void vkrDevice_Shutdown(vkrSys* vkr)
{
    if (vkr)
    {
        vkrDestroyQueues();
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
    if (g_vkr.dev)
    {
        VkCheck(vkDeviceWaitIdle(g_vkr.dev));
    }
}

// ----------------------------------------------------------------------------

u32 vkrEnumPhysicalDevices(
    VkInstance inst,
    VkPhysicalDevice** devicesOut,
    vkrProps** propsOut,
    vkrFeats** featsOut,
    vkrDevExts** extsOut)
{
    ASSERT(inst);

    u32 count = 0;
    VkCheck(vkEnumeratePhysicalDevices(inst, &count, NULL));

    if (count && devicesOut)
    {
        VkPhysicalDevice* devices = Temp_Calloc(sizeof(devices[0]) * count);
        VkCheck(vkEnumeratePhysicalDevices(inst, &count, devices));
        *devicesOut = devices;

        if (propsOut)
        {
            vkrProps* props = Temp_Calloc(sizeof(props[0]) * count);
            *propsOut = props;
            for (u32 i = 0; i < count; ++i)
            {
                ASSERT(devices[i]);
                props[i].phdev.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
#               if VKR_RT
                {
                    props[i].phdev.pNext = &props[i].accstr;
                    props[i].accstr.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
                    props[i].accstr.pNext = &props[i].rtpipe;
                    props[i].rtpipe.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
                    props[i].rtpipe.pNext = NULL;
                }
#               endif // VKR_RT
                vkGetPhysicalDeviceProperties2(devices[i], &props[i].phdev);
            }
        }

        if (featsOut)
        {
            vkrFeats* feats = Temp_Calloc(sizeof(feats[0]) * count);
            *featsOut = feats;
            for (u32 i = 0; i < count; ++i)
            {
                ASSERT(devices[i]);
                feats[i].phdev.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
#               if VKR_RT
                {
                    feats[i].phdev.pNext = &feats[i].accstr;
                    feats[i].accstr.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
                    feats[i].accstr.pNext = &feats[i].rtpipe;
                    feats[i].rtpipe.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
                    feats[i].rtpipe.pNext = &feats[i].rquery;
                    feats[i].rquery.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
                    feats[i].rquery.pNext = NULL;
                }
#               endif // VKR_RT
                vkGetPhysicalDeviceFeatures2(devices[i], &feats[i].phdev);
            }
        }

        if (extsOut)
        {
            vkrDevExts* exts = Temp_Calloc(sizeof(exts[0]) * count);
            *extsOut = exts;
            for (u32 i = 0; i < count; ++i)
            {
                ASSERT(devices[i]);
                vkrDevExts_New(&exts[i], devices[i]);
            }
        }
    }

    return count;
}

VkPhysicalDevice vkrSelectPhysicalDevice(
    const vkrWindow* window,
    vkrProps* propsOut,
    vkrFeats* featuresOut,
    vkrDevExts* extsOut)
{
    VkInstance inst = g_vkr.inst;
    ASSERT(inst);
    ASSERT(window);
    VkSurfaceKHR surface = window->surface;
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
    vkrProps* props = NULL;
    vkrFeats* feats = NULL;
    vkrDevExts* exts = NULL;
    const u32 count = vkrEnumPhysicalDevices(inst, &devices, &props, &feats, &exts);
    vkrPhDevScore* scores = Temp_Calloc(sizeof(scores[0]) * count);

    for (u32 iDevice = 0; iDevice < count; ++iDevice)
    {
        scores[iDevice].hasRequiredExts = vkrDevExts_ReqEval(&exts[iDevice]);
        scores[iDevice].rtScore = vkrDevExts_RtEval(&exts[iDevice]);
        scores[iDevice].extScore = vkrDevExts_OptEval(&exts[iDevice]);
        scores[iDevice].featScore = vkrFeats_Eval(&feats[iDevice]);
        scores[iDevice].propScore = vkrProps_Eval(&props[iDevice]);

        vkrQueueSupport support = vkrQueryQueueSupport(devices[iDevice], surface);
        bool hasQueueSupport = true;
        for (i32 iQueue = 0; iQueue < NELEM(support.family); ++iQueue)
        {
            hasQueueSupport &= (support.family[iQueue] >= 0);
        }
        scores[iDevice].hasQueueSupport = hasQueueSupport;
    }

    i32 chosenDev = -1;
    for (u32 iDevice = 0; iDevice < count; ++iDevice)
    {
        if (!scores[iDevice].hasRequiredExts)
        {
            continue;
        }
        if (!scores[iDevice].hasQueueSupport)
        {
            continue;
        }
        if (chosenDev < 0)
        {
            chosenDev = iDevice;
            continue;
        }

        i32 cmp = scores[chosenDev].rtScore - scores[iDevice].rtScore;
        if (cmp != 0)
        {
            chosenDev = cmp < 0 ? iDevice : chosenDev;
            continue;
        }
        cmp = scores[chosenDev].extScore - scores[iDevice].extScore;
        if (cmp != 0)
        {
            chosenDev = cmp < 0 ? iDevice : chosenDev;
            continue;
        }
        cmp = scores[chosenDev].featScore - scores[iDevice].featScore;
        if (cmp != 0)
        {
            chosenDev = cmp < 0 ? iDevice : chosenDev;
            continue;
        }
        cmp = scores[chosenDev].propScore - scores[iDevice].propScore;
        if (cmp != 0)
        {
            chosenDev = cmp < 0 ? iDevice : chosenDev;
            continue;
        }
    }

    if (chosenDev >= 0)
    {
        if (propsOut)
        {
            *propsOut = props[chosenDev];
        }
        if (featuresOut)
        {
            *featuresOut = feats[chosenDev];
        }
        if (extsOut)
        {
            *extsOut = exts[chosenDev];
        }
        return devices[chosenDev];
    }

    return NULL;
}

VkDevice vkrCreateDevice(
    const vkrWindow* window,
    StrList extensions,
    StrList layers)
{
    VkPhysicalDevice phdev = g_vkr.phdev;
    ASSERT(phdev);
    ASSERT(window);
    VkSurfaceKHR surface = window->surface;
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

    const vkrFeats* feats = &g_vkrFeats;

    vkrFeats reqFeats =
    {
        .phdev =
        {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            .features =
            {
                .fullDrawIndexUint32 = feats->phdev.features.fullDrawIndexUint32,
                .samplerAnisotropy = feats->phdev.features.samplerAnisotropy,
                .textureCompressionBC = feats->phdev.features.textureCompressionBC,
                .independentBlend = feats->phdev.features.independentBlend,

                .fillModeNonSolid = feats->phdev.features.fillModeNonSolid,
                .wideLines = feats->phdev.features.wideLines,
                .largePoints = feats->phdev.features.largePoints,

                .fragmentStoresAndAtomics = feats->phdev.features.fragmentStoresAndAtomics,
                .shaderInt64 = feats->phdev.features.shaderInt64,
                .shaderInt16 = feats->phdev.features.shaderInt16,
                .shaderStorageImageExtendedFormats = feats->phdev.features.shaderStorageImageExtendedFormats,

                .shaderUniformBufferArrayDynamicIndexing = feats->phdev.features.shaderUniformBufferArrayDynamicIndexing,
                .shaderStorageBufferArrayDynamicIndexing = feats->phdev.features.shaderStorageBufferArrayDynamicIndexing,
                .shaderSampledImageArrayDynamicIndexing = feats->phdev.features.shaderSampledImageArrayDynamicIndexing,
                .shaderStorageImageArrayDynamicIndexing = feats->phdev.features.shaderStorageImageArrayDynamicIndexing,
                .imageCubeArray = feats->phdev.features.imageCubeArray,
            },
        },
#       if VKR_RT
        .accstr =
        {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
            .accelerationStructure = feats->accstr.accelerationStructure,
            .accelerationStructureIndirectBuild = feats->accstr.accelerationStructureIndirectBuild,
            .accelerationStructureHostCommands = feats->accstr.accelerationStructureHostCommands,
        },
        .rtpipe =
        {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
            .rayTracingPipeline = feats->rtpipe.rayTracingPipeline,
            .rayTracingPipelineTraceRaysIndirect = feats->rtpipe.rayTracingPipelineTraceRaysIndirect,
            .rayTraversalPrimitiveCulling = feats->rtpipe.rayTraversalPrimitiveCulling,
        },
        .rquery =
        {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR,
            .rayQuery = feats->rquery.rayQuery,
        },
#       endif // VKR_RT
    };
#if VKR_RT
    if (g_vkrDevExts.KHR_ray_tracing_pipeline)
    {
        reqFeats.phdev.pNext = &reqFeats.accstr;
        reqFeats.accstr.pNext = &reqFeats.rtpipe;
        reqFeats.rtpipe.pNext = &reqFeats.rquery;
        reqFeats.rquery.pNext = NULL;
    }
#endif // VKR_RT

    const VkDeviceCreateInfo devInfo =
    {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &reqFeats.phdev,
        .queueCreateInfoCount = familyCount,
        .pQueueCreateInfos = queueInfos,
        .enabledLayerCount = layers.count,
        .ppEnabledLayerNames = layers.ptr,
        .enabledExtensionCount = extensions.count,
        .ppEnabledExtensionNames = extensions.ptr,
    };

    VkDevice device = NULL;
    VkCheck(vkCreateDevice(g_vkr.phdev, &devInfo, NULL, &device));
    ASSERT(device);

    g_vkrFeats = reqFeats;

    StrList_Del(&extensions);
    StrList_Del(&layers);

    return device;
}
