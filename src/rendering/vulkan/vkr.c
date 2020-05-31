#include "rendering/vulkan/vkr.h"
#include <volk/volk.h>
#include <GLFW/glfw3.h>
#include "allocator/allocator.h"
#include "common/console.h"
#include "common/stringutil.h"
#include "containers/strlist.h"
#include <string.h>

#define VkCheck(expr) do { VkResult _res = (expr); ASSERT(_res == VK_SUCCESS); } while(0)

static void vkrListInstExtensions(void);
static void vkrListDevExtensions(void);
static strlist_t vkrGetLayers(void);
static strlist_t vkrGetInstExtensions(void);
static strlist_t vkrGetDevExtensions(void);
static i32 vkrFindExtension(
    const VkExtensionProperties* props,
    u32 count,
    const char* extName);
static bool vkrTryAddExtension(
    strlist_t* list,
    const VkExtensionProperties* props,
    u32 propCount,
    const char* extName);
static VkInstance vkrCreateInstance(strlist_t extensions, strlist_t layers);
static VkDebugUtilsMessengerEXT vkrCreateDebugMessenger(void);
static VKAPI_ATTR VkBool32 VKAPI_CALL vkrOnVulkanMessage(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData);
static VkPhysicalDevice vkrSelectPhysicalDevice(
    VkPhysicalDeviceProperties* propsOut, VkPhysicalDeviceFeatures* featuresOut);
static i32 vkrFindQueueFamily(
    VkPhysicalDevice phdev,
    u32 requestedFlags,
    VkQueueFamilyProperties* propsOut);
static VkDevice vkrCreateDevice(strlist_t extensions, strlist_t layers, VkQueue* gfxOut, VkQueue* xferOut, VkQueue* compOut);

GLFWwindow* g_vkrwindow;
vkrDevice g_vkr;

static VkDebugUtilsMessengerEXT ms_messenger;
static VkPhysicalDeviceProperties ms_phdevProps;
static VkPhysicalDeviceFeatures ms_phdevFeats;

const VkPhysicalDeviceFeatures* vkrPhDevFeats(void) { return &ms_phdevFeats; }
const VkPhysicalDeviceProperties* vkrPhDevProps(void) { return &ms_phdevProps; }
const VkPhysicalDeviceLimits* vkrPhDevLimits(void) { return &ms_phdevProps.limits; }

void vkr_init(void)
{
    VkCheck(volkInitialize());

    i32 rv = glfwInit();
    ASSERT(rv == GLFW_TRUE);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    g_vkrwindow = glfwCreateWindow(320, 240, "pimvk", NULL, NULL);
    ASSERT(g_vkrwindow);

    vkrListInstExtensions();
    g_vkr.inst = vkrCreateInstance(vkrGetInstExtensions(), vkrGetLayers());
    ASSERT(g_vkr.inst);
    volkLoadInstance(g_vkr.inst);

    ms_messenger = vkrCreateDebugMessenger();

    g_vkr.phdev = vkrSelectPhysicalDevice(&ms_phdevProps, &ms_phdevFeats);
    ASSERT(g_vkr.phdev);
    vkrListDevExtensions();

    g_vkr.dev = vkrCreateDevice(
        vkrGetDevExtensions(),
        vkrGetLayers(),
        &g_vkr.gfxq,
        &g_vkr.xferq,
        &g_vkr.compq);
    ASSERT(g_vkr.dev);
    ASSERT(g_vkr.gfxq);
    ASSERT(g_vkr.xferq);
    ASSERT(g_vkr.compq);
    volkLoadDevice(g_vkr.dev);

}

void vkr_update(void)
{

}

void vkr_shutdown(void)
{
    vkDestroyDevice(g_vkr.dev, NULL);
    g_vkr.dev = NULL;

#ifdef _DEBUG
    vkDestroyDebugUtilsMessengerEXT(g_vkr.inst, ms_messenger, NULL);
    ms_messenger = NULL;
#endif // _DEBUG

    vkDestroyInstance(g_vkr.inst, NULL);
    g_vkr.inst = NULL;

    glfwDestroyWindow(g_vkrwindow);
    g_vkrwindow = NULL;
}

static VkDebugUtilsMessengerEXT vkrCreateDebugMessenger()
{
    ASSERT(g_vkr.inst);

    VkDebugUtilsMessengerEXT messenger = NULL;
#ifdef _DEBUG
    VkCheck(vkCreateDebugUtilsMessengerEXT(g_vkr.inst,
        &(VkDebugUtilsMessengerCreateInfoEXT)
    {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT,
            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = vkrOnVulkanMessage,
    }, NULL, &messenger));
#endif // _DEBUG
    return messenger;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL vkrOnVulkanMessage(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    LogSev sev = LogSev_Error;
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        sev = LogSev_Error;
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        sev = LogSev_Warning;
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        sev = LogSev_Info;
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
    {
        sev = LogSev_Verbose;
    }

    bool shouldLog =
        (sev <= LogSev_Warning) ||
        (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT);

    if (shouldLog)
    {
        con_logf(sev, "Vk", pCallbackData->pMessage);
    }

    return VK_FALSE;
}

static void vkrListInstExtensions(void)
{
    u32 count = 0;
    VkExtensionProperties* props = NULL;
    VkCheck(vkEnumerateInstanceExtensionProperties(NULL, &count, NULL));
    TempReserve(props, count);
    VkCheck(vkEnumerateInstanceExtensionProperties(NULL, &count, props));

    con_logf(LogSev_Info, "Vk", "%d available instance extensions", count);
    for (u32 i = 0; i < count; ++i)
    {
        con_logf(LogSev_Info, "Vk", props[i].extensionName);
    }
}

static void vkrListDevExtensions(void)
{
    ASSERT(g_vkr.phdev);

    u32 count = 0;
    VkExtensionProperties* props = NULL;
    VkCheck(vkEnumerateDeviceExtensionProperties(g_vkr.phdev, NULL, &count, NULL));
    TempReserve(props, count);
    VkCheck(vkEnumerateDeviceExtensionProperties(g_vkr.phdev, NULL, &count, props));

    con_logf(LogSev_Info, "Vk", "%d available device extensions", count);
    for (u32 i = 0; i < count; ++i)
    {
        con_logf(LogSev_Info, "Vk", props[i].extensionName);
    }
}

static strlist_t vkrGetLayers(void)
{
    strlist_t list;
    strlist_new(&list, EAlloc_Temp);
#ifdef _DEBUG
    strlist_add(&list, "VK_LAYER_KHRONOS_validation");
#endif // _DEBUG
    return list;
}

static strlist_t vkrGetInstExtensions(void)
{
    strlist_t list;
    strlist_new(&list, EAlloc_Temp);

    u32 count = 0;
    VkExtensionProperties* props = NULL;
    VkCheck(vkEnumerateInstanceExtensionProperties(NULL, &count, NULL));
    TempReserve(props, count);
    VkCheck(vkEnumerateInstanceExtensionProperties(NULL, &count, props));

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

static i32 vkrFindExtension(
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

static bool vkrTryAddExtension(
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
    con_logf(LogSev_Warning, "Vk", "Failed to load extension '%s'", extName);
    return false;
}

static strlist_t vkrGetDevExtensions(void)
{
    ASSERT(g_vkr.phdev);

    u32 propCount = 0;
    VkExtensionProperties* props = NULL;
    VkCheck(vkEnumerateDeviceExtensionProperties(g_vkr.phdev, NULL, &propCount, NULL));
    TempReserve(props, propCount);
    VkCheck(vkEnumerateDeviceExtensionProperties(g_vkr.phdev, NULL, &propCount, props));

    strlist_t list;
    strlist_new(&list, EAlloc_Temp);

    // https://www.khronos.org/registry/vulkan/specs/1.0-wsi_extensions/html/vkspec.html#VK_KHR_swapchain
    vkrTryAddExtension(
        &list, props, propCount, VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    // https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#VK_KHR_performance_query
    // requires flipping a switch in nvidia driver settings, or it won't be listed
    vkrTryAddExtension(
        &list, props, propCount, VK_KHR_PERFORMANCE_QUERY_EXTENSION_NAME);

    // https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#VK_EXT_conditional_rendering
    vkrTryAddExtension(
        &list, props, propCount, VK_EXT_CONDITIONAL_RENDERING_EXTENSION_NAME);

    return list;
}

static VkInstance vkrCreateInstance(strlist_t extensions, strlist_t layers)
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

static VkPhysicalDevice vkrSelectPhysicalDevice(
    VkPhysicalDeviceProperties* propsOut,
    VkPhysicalDeviceFeatures* featuresOut)
{
    ASSERT(g_vkr.inst);

    if (propsOut)
    {
        memset(propsOut, 0, sizeof(*propsOut));
    }
    if (featuresOut)
    {
        memset(featuresOut, 0, sizeof(*featuresOut));
    }

    u32 count = 0;
    VkPhysicalDevice* devices = NULL;
    VkCheck(vkEnumeratePhysicalDevices(g_vkr.inst, &count, NULL));
    TempReserve(devices, count);
    VkCheck(vkEnumeratePhysicalDevices(g_vkr.inst, &count, devices));

    VkPhysicalDeviceProperties* props = tmp_calloc(sizeof(props[0]) * count);
    VkPhysicalDeviceFeatures* features = tmp_calloc(sizeof(features[0]) * count);
    for (u32 i = 0; i < count; ++i)
    {
        vkGetPhysicalDeviceProperties(devices[i], props + i);
        vkGetPhysicalDeviceFeatures(devices[i], features + i);
    }

    i32 c = -1;
    for (u32 i = 0; i < count; ++i)
    {
        VkPhysicalDevice phdev = devices[i];
        i32 iGfx = vkrFindQueueFamily(phdev, VK_QUEUE_GRAPHICS_BIT, NULL);
        if (iGfx == -1)
        {
            continue;
        }
        i32 iXfer = vkrFindQueueFamily(phdev, VK_QUEUE_TRANSFER_BIT, NULL);
        if (iXfer == -1)
        {
            continue;
        }
        i32 iCompute = vkrFindQueueFamily(phdev, VK_QUEUE_COMPUTE_BIT, NULL);
        if (iCompute == -1)
        {
            continue;
        }
        if (c == -1)
        {
            c = i;
            continue;
        }
        i32 limCmp = memcmp(&props[i].limits, &props[c].limits, sizeof(props[0].limits));
        i32 featCmp = memcmp(&features[i], &features[c], sizeof(features[0]));
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
            *featuresOut = features[c];
        }
        return devices[c];
    }
    return NULL;
}

static i32 vkrFindQueueFamily(
    VkPhysicalDevice phdev,
    u32 requestedFlags,
    VkQueueFamilyProperties* propsOut)
{
    ASSERT(phdev);
    ASSERT(requestedFlags);

    u32 count = 0;
    VkQueueFamilyProperties* props = NULL;
    vkGetPhysicalDeviceQueueFamilyProperties(phdev, &count, NULL);
    props = tmp_calloc(sizeof(props[0]) * count);
    vkGetPhysicalDeviceQueueFamilyProperties(phdev, &count, props);

    i32 c = -1;
    for (u32 i = 0; i < count; ++i)
    {
        u32 iFlags = props[i].queueFlags;
        bool hasAll = (iFlags & requestedFlags) == requestedFlags;
        if (!hasAll)
        {
            continue;
        }
        if (c == -1)
        {
            c = i;
            continue;
        }
        if (props[i].queueCount > props[c].queueCount)
        {
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
    }
    return c;
}

static VkDevice vkrCreateDevice(
    strlist_t extensions,
    strlist_t layers,
    VkQueue* gfxOut,
    VkQueue* xferOut,
    VkQueue* compOut)
{
    ASSERT(g_vkr.phdev);
    ASSERT(gfxOut);
    ASSERT(xferOut);
    ASSERT(compOut);

    u32 queueBits[] =
    {
        VK_QUEUE_GRAPHICS_BIT,
        VK_QUEUE_TRANSFER_BIT,
        VK_QUEUE_COMPUTE_BIT,
    };
    float queuePrios[] =
    {
        1.0f,
        0.666f,
        0.666f,
    };
    SASSERT(NELEM(queuePrios) == NELEM(queueBits));

    i32 queueFams[NELEM(queueBits)];
    memset(queueFams, 0, sizeof(queueFams));

    VkQueueFamilyProperties queueProps[NELEM(queueBits)];
    memset(queueProps, 0, sizeof(queueProps));

    VkDeviceQueueCreateInfo queueInfos[NELEM(queueBits)];
    memset(queueInfos, 0, sizeof(queueInfos));

    // find queue for each type we want
    for (i32 i = 0; i < NELEM(queueBits); ++i)
    {
        queueFams[i] = vkrFindQueueFamily(g_vkr.phdev, queueBits[i], &queueProps[i]);
        ASSERT(queueFams[i] >= 0);
        queueInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfos[i].queueFamilyIndex = queueFams[i];
        queueInfos[i].queueCount = 1;
        queueInfos[i].pQueuePriorities = &queuePrios[i];
    }

    // remove duplicate queues
    for (i32 i = 0; i < NELEM(queueBits); ++i)
    {
        u32 iFam = queueInfos[i].queueFamilyIndex;
        for (i32 j = i + 1; j < NELEM(queueBits); ++j)
        {
            u32 jFam = queueInfos[j].queueFamilyIndex;
            if (jFam == iFam)
            {
                queueInfos[j].queueCount = 0;
                queueBits[i] |= queueBits[j];
                queuePrios[i] = queuePrios[i] > queuePrios[j] ? queuePrios[i] : queuePrios[j];
            }
        }
    }

    // remove holes in array
    u32 queueCount = 0;
    for (i32 i = 0; i < NELEM(queueBits); ++i)
    {
        if (queueInfos[i].queueCount > 0)
        {
            queuePrios[queueCount] = queuePrios[i];
            queueInfos[queueCount] = queueInfos[i];
            ++queueCount;
        }
    }

    // features we intend to use
    const VkPhysicalDeviceFeatures phDevFeatures =
    {
        // basic rasterizer features
        .fillModeNonSolid = ms_phdevFeats.fillModeNonSolid,
        .wideLines = ms_phdevFeats.wideLines,
        .largePoints = ms_phdevFeats.largePoints,
        .depthClamp = ms_phdevFeats.depthClamp,
        .depthBounds = ms_phdevFeats.depthBounds,

        // anisotropic sampling
        .samplerAnisotropy = ms_phdevFeats.samplerAnisotropy,

        // block compression
        .textureCompressionETC2 = ms_phdevFeats.textureCompressionETC2,
        .textureCompressionASTC_LDR = ms_phdevFeats.textureCompressionASTC_LDR,
        .textureCompressionBC = ms_phdevFeats.textureCompressionBC,

        // throughput statistics
        .pipelineStatisticsQuery = ms_phdevFeats.pipelineStatisticsQuery,

        // indexing resources in shaders
        .shaderUniformBufferArrayDynamicIndexing = ms_phdevFeats.shaderUniformBufferArrayDynamicIndexing,
        .shaderStorageBufferArrayDynamicIndexing = ms_phdevFeats.shaderStorageBufferArrayDynamicIndexing,
        .shaderSampledImageArrayDynamicIndexing = ms_phdevFeats.shaderSampledImageArrayDynamicIndexing,
        .shaderStorageImageArrayDynamicIndexing = ms_phdevFeats.shaderStorageImageArrayDynamicIndexing,

        // cubemap arrays
        .imageCubeArray = ms_phdevFeats.imageCubeArray,

        // indirect rendering
        .multiDrawIndirect = ms_phdevFeats.multiDrawIndirect,
    };

    const VkDeviceCreateInfo devInfo =
    {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = queueCount,
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
        vkGetDeviceQueue(device, queueFams[0], 0, gfxOut);
        vkGetDeviceQueue(device, queueFams[1], 0, xferOut);
        vkGetDeviceQueue(device, queueFams[2], 0, compOut);
    }

    return device;
}
