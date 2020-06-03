#include "rendering/vulkan/vkr.h"
#include "rendering/vulkan/vkrcompiler.h"
#include <volk/volk.h>
#include <GLFW/glfw3.h>
#include "allocator/allocator.h"
#include "common/console.h"
#include "common/stringutil.h"
#include "containers/strlist.h"
#include "rendering/constants.h"
#include <string.h>

#define VkCheck(expr) do { VkResult _res = (expr); ASSERT(_res == VK_SUCCESS); } while(0)

typedef struct vkrQueueSupport
{
    i32 family[vkrQueueId_COUNT];
    u32 count;
    VkQueueFamilyProperties* props;
} vkrQueueSupport;

typedef struct vkrSwapchainSupport
{
    VkSurfaceCapabilitiesKHR caps;
    u32 formatCount;
    VkSurfaceFormatKHR* formats;
    u32 modeCount;
    VkPresentModeKHR* modes;
} vkrSwapchainSupport;

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

vkr_t g_vkr;
vkrDisplay g_vkrdisp;

static VkDebugUtilsMessengerEXT ms_messenger;
static VkPhysicalDeviceProperties ms_phdevProps;
static VkPhysicalDeviceFeatures ms_phdevFeats;
static VkQueueFamilyProperties ms_queueProps[vkrQueueId_COUNT];

const VkPhysicalDeviceFeatures* vkrPhDevFeats(void) { return &ms_phdevFeats; }
const VkPhysicalDeviceProperties* vkrPhDevProps(void) { return &ms_phdevProps; }
const VkPhysicalDeviceLimits* vkrPhDevLimits(void) { return &ms_phdevProps.limits; }

static VkExtensionProperties* vkrEnumInstExtensions(u32* countOut);
static VkExtensionProperties* vkrEnumDevExtensions(
    VkPhysicalDevice phdev,
    u32* countOut);

static u32 vkrEnumPhysicalDevices(
    VkInstance inst,
    VkPhysicalDevice** pDevices, // optional
    VkPhysicalDeviceFeatures** pFeatures, // optional
    VkPhysicalDeviceProperties** pProps); // optional

static void vkrListInstExtensions(void);
static void vkrListDevExtensions(VkPhysicalDevice phdev);
static strlist_t vkrGetLayers(void);
static strlist_t vkrGetInstExtensions(void);
static strlist_t vkrGetDevExtensions(VkPhysicalDevice phdev);
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

static VkSurfaceKHR vkrCreateSurface(VkInstance inst, GLFWwindow* win);

static VkPhysicalDevice vkrSelectPhysicalDevice(
    VkPhysicalDeviceProperties* propsOut, // optional
    VkPhysicalDeviceFeatures* featuresOut); // optional

static vkrQueueSupport vkrQueryQueueSupport(
    VkPhysicalDevice phdev,
    VkSurfaceKHR surf);

static VkDevice vkrCreateDevice(
    strlist_t extensions,
    strlist_t layers,
    VkQueue* queuesOut, // optional
    VkQueueFamilyProperties* propsOut); // optional

static vkrSwapchainSupport vkrQuerySwapchainSupport(
    VkPhysicalDevice phdev,
    VkSurfaceKHR surf);
static i32 vkrSelectSwapFormat(const VkSurfaceFormatKHR* formats, u32 formatCount);
static i32 vkrSelectSwapMode(const VkPresentModeKHR* modes, u32 count);

static void vkrCreateSwapchain(VkSurfaceKHR surf,  VkSwapchainKHR prev);

void vkr_init(void)
{
    memset(&g_vkr, 0, sizeof(g_vkr));
    memset(&g_vkrdisp, 0, sizeof(g_vkrdisp));

    VkCheck(volkInitialize());

    i32 rv = glfwInit();
    ASSERT(rv == GLFW_TRUE);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    g_vkrdisp.win = glfwCreateWindow(320, 240, "pimvk", NULL, NULL);
    ASSERT(g_vkrdisp.win);

    vkrListInstExtensions();
    g_vkr.inst = vkrCreateInstance(vkrGetInstExtensions(), vkrGetLayers());
    ASSERT(g_vkr.inst);

    volkLoadInstance(g_vkr.inst);

    ms_messenger = vkrCreateDebugMessenger();

    g_vkrdisp.surf = vkrCreateSurface(g_vkr.inst, g_vkrdisp.win);
    ASSERT(g_vkrdisp.surf);

    g_vkr.phdev = vkrSelectPhysicalDevice(&ms_phdevProps, &ms_phdevFeats);
    ASSERT(g_vkr.phdev);

    vkrListDevExtensions(g_vkr.phdev);

    g_vkr.dev = vkrCreateDevice(
        vkrGetDevExtensions(g_vkr.phdev),
        vkrGetLayers(),
        g_vkr.queues,
        ms_queueProps);
    ASSERT(g_vkr.dev);

    volkLoadDevice(g_vkr.dev);

    vkrCreateSwapchain(g_vkrdisp.surf, NULL);
    ASSERT(g_vkrdisp.swap);

    const vkrCompileInput input =
    {
        .filename="example.hlsl",
        .entrypoint="PSMain",
        .text="struct PSInput{float4 color : COLOR; }; float4 PSMain(PSInput input) : SV_TARGET { return input.color; }",
        .type=vkrShaderType_Fragment,
    };
    vkrCompileOutput output = {0};
    vkrCompile(&input, &output);
    if (output.errors)
    {
        con_logf(LogSev_Error, "Vkc", "%s", output.errors);
    }
    if (output.disassembly)
    {
        con_logf(LogSev_Info, "Vkc", "%s", output.disassembly);
    }
}

void vkr_update(void)
{

}

void vkr_shutdown(void)
{
    for (u32 i = 0; i < g_vkrdisp.imgCount; ++i)
    {
        vkDestroyImageView(g_vkr.dev, g_vkrdisp.views[i], NULL);
    }
    vkDestroySwapchainKHR(g_vkr.dev, g_vkrdisp.swap, NULL);
    g_vkrdisp.swap = NULL;

    vkDestroyDevice(g_vkr.dev, NULL);
    g_vkr.dev = NULL;

    vkDestroySurfaceKHR(g_vkr.inst, g_vkrdisp.surf, NULL);
    g_vkrdisp.surf = NULL;

#ifdef _DEBUG
    vkDestroyDebugUtilsMessengerEXT(g_vkr.inst, ms_messenger, NULL);
    ms_messenger = NULL;
#endif // _DEBUG

    vkDestroyInstance(g_vkr.inst, NULL);
    g_vkr.inst = NULL;

    glfwDestroyWindow(g_vkrdisp.win);
    g_vkrdisp.win = NULL;
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

static VkExtensionProperties* vkrEnumInstExtensions(u32* countOut)
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

static VkExtensionProperties* vkrEnumDevExtensions(
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
    return false;
}

static void vkrListInstExtensions(void)
{
    u32 count = 0;
    VkExtensionProperties* props = vkrEnumInstExtensions(&count);
    con_logf(LogSev_Info, "Vk", "%d available instance extensions", count);
    for (u32 i = 0; i < count; ++i)
    {
        con_logf(LogSev_Info, "Vk", props[i].extensionName);
    }
}

static void vkrListDevExtensions(VkPhysicalDevice phdev)
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

static strlist_t vkrGetDevExtensions(VkPhysicalDevice phdev)
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

static VkSurfaceKHR vkrCreateSurface(VkInstance inst, GLFWwindow* win)
{
    ASSERT(inst);
    ASSERT(win);

    VkSurfaceKHR surf = NULL;
    VkCheck(glfwCreateWindowSurface(inst, win, NULL, &surf));

    return surf;
}

static u32 vkrEnumPhysicalDevices(
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

static VkPhysicalDevice vkrSelectPhysicalDevice(
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

static void vkrSelectFamily(i32* fam, i32 i, const VkQueueFamilyProperties* props)
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

static VkQueueFamilyProperties* vkrEnumQueueFamilyProperties(
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

static vkrQueueSupport vkrQueryQueueSupport(VkPhysicalDevice phdev, VkSurfaceKHR surf)
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

static VkDevice vkrCreateDevice(
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

static vkrSwapchainSupport vkrQuerySwapchainSupport(
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

static i32 vkrSelectSwapFormat(const VkSurfaceFormatKHR* formats, u32 count)
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
    return -1;
}

// pim prefers low latency over tear protection
static const VkPresentModeKHR kPreferredPresentModes[] =
{
    VK_PRESENT_MODE_MAILBOX_KHR,        // single-entry overwrote queue; good latency
    VK_PRESENT_MODE_IMMEDIATE_KHR,      // no queue; best latency, bad tearing
    VK_PRESENT_MODE_FIFO_RELAXED_KHR,   // multi-entry adaptive queue; bad latency
    VK_PRESENT_MODE_FIFO_KHR ,          // multi-entry queue; bad latency
};

static i32 vkrSelectSwapMode(const VkPresentModeKHR* modes, u32 count)
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
    return -1;
}

static VkExtent2D vkrSelectSwapExtent(const VkSurfaceCapabilitiesKHR* caps)
{
    if (caps->currentExtent.width != ~0u)
    {
        return caps->currentExtent;
    }
    VkExtent2D ext = { kDrawWidth, kDrawHeight };
    VkExtent2D minExt = caps->minImageExtent;
    VkExtent2D maxExt = caps->maxImageExtent;
    ext.width = ext.width > minExt.width ? ext.width : minExt.width;
    ext.height = ext.height > minExt.height ? ext.height : minExt.height;
    ext.width = ext.width < maxExt.width ? ext.width : maxExt.width;
    ext.height = ext.height < maxExt.height ? ext.height : maxExt.height;
    return ext;
}

static void vkrCreateSwapchain(VkSurfaceKHR surf, VkSwapchainKHR prev)
{
    VkPhysicalDevice phdev = g_vkr.phdev;
    VkDevice dev = g_vkr.dev;
    ASSERT(phdev);
    ASSERT(dev);
    ASSERT(surf);

    vkrSwapchainSupport sup = vkrQuerySwapchainSupport(phdev, surf);
    vkrQueueSupport qsup = vkrQueryQueueSupport(phdev, surf);

    i32 iFormat = vkrSelectSwapFormat(sup.formats, sup.formatCount);
    ASSERT(iFormat != -1);
    i32 iMode = vkrSelectSwapMode(sup.modes, sup.modeCount);
    ASSERT(iMode != -1);
    VkExtent2D ext = vkrSelectSwapExtent(&sup.caps);

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

    g_vkrdisp.swap = swap;
    g_vkrdisp.format = sup.formats[iFormat].format;
    g_vkrdisp.colorSpace = sup.formats[iFormat].colorSpace;
    g_vkrdisp.width = ext.width;
    g_vkrdisp.height = ext.height;

    VkCheck(vkGetSwapchainImagesKHR(dev, swap, &imgCount, NULL));
    PermReserve(g_vkrdisp.images, imgCount);
    PermReserve(g_vkrdisp.views, imgCount);
    VkCheck(vkGetSwapchainImagesKHR(dev, swap, &imgCount, g_vkrdisp.images));
    g_vkrdisp.imgCount = imgCount;

    for (u32 i = 0; i < imgCount; ++i)
    {
        const VkImageViewCreateInfo viewInfo =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = g_vkrdisp.images[i],
            .format = g_vkrdisp.format,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.levelCount = 1,
            .subresourceRange.layerCount = 1,
        };
        g_vkrdisp.views[i] = NULL;
        VkCheck(vkCreateImageView(dev, &viewInfo, NULL, g_vkrdisp.views + i));
        ASSERT(g_vkrdisp.views[i]);
    }
}
