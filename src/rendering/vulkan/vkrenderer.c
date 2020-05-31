#include "rendering/vulkan/vkrenderer.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "allocator/allocator.h"
#include "common/console.h"
#include "common/stringutil.h"
#include "containers/strlist.h"
#include <string.h>

#define VkCheck(expr) do { VkResult _res = (expr); ASSERT(_res == VK_SUCCESS); } while(0)

static const VkApplicationInfo kAppInfo =
{
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pNext = NULL,
    .pApplicationName = "pimquake",
    .applicationVersion = 1,
    .pEngineName = "pim",
    .engineVersion = 1,
    .apiVersion = VK_API_VERSION_1_1,
};

static void ListExtensions(void);
static strlist_t GetExtensions(void);
static strlist_t GetLayers(void);
static VkInstance CreateInstance(strlist_t extensions, strlist_t layers);

static VkDebugUtilsMessengerEXT CreateDebugMessenger(VkInstance instance);
static void DestroyDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT messenger);
static VKAPI_ATTR VkBool32 VKAPI_CALL OnVulkanMessage(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData);

static GLFWwindow* ms_window;
static VkInstance ms_instance;
static VkDebugUtilsMessengerEXT ms_messenger;

void vkrenderer_init(void)
{
    i32 rv = glfwInit();
    ASSERT(rv == GLFW_TRUE);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    ms_window = glfwCreateWindow(320, 240, "pimvk", NULL, NULL);
    ASSERT(ms_window);

    ListExtensions();
    ms_instance = CreateInstance(GetExtensions(), GetLayers());
    ms_messenger = CreateDebugMessenger(ms_instance);
}

void vkrenderer_update(void)
{

}

void vkrenderer_shutdown(void)
{
    DestroyDebugMessenger(ms_instance, ms_messenger);

    vkDestroyInstance(ms_instance, NULL);
    ms_instance = NULL;

    glfwDestroyWindow(ms_window);
    ms_window = NULL;
}

static VkDebugUtilsMessengerEXT CreateDebugMessenger(VkInstance instance)
{
    ASSERT(instance);
    VkDebugUtilsMessengerEXT messenger = NULL;
#ifdef _DEBUG
    PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    ASSERT(func);
    if (func)
    {
        VkCheck(func(instance,
            &(VkDebugUtilsMessengerCreateInfoEXT)
        {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
                .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT,
                .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
                .pfnUserCallback = OnVulkanMessage,
        }, NULL, &messenger));
    }
#endif // _DEBUG
    return messenger;
}

static void DestroyDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT messenger)
{
    ASSERT(instance);
#ifdef _DEBUG
    if (messenger)
    {
        PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        ASSERT(func);
        if (func)
        {
            func(instance, messenger, NULL);
        }
    }
#endif // _DEBUG
}

static VKAPI_ATTR VkBool32 VKAPI_CALL OnVulkanMessage(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    const char* sevTag = "";
    u32 color = C32_GRAY;
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        sevTag = "ERROR";
        color = C32_RED;
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        sevTag = "WARN";
        color = C32_YELLOW;
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        sevTag = "INFO";
        color = C32_WHITE;
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
    {
        sevTag = "VERBOSE";
        color = C32_GRAY;
    }

    con_printf(color, "[Vk][%s] %s", sevTag, pCallbackData->pMessage);
    return VK_FALSE;
}

static void ListExtensions(void)
{
    u32 instExtPropCount = 0;
    VkExtensionProperties* instExtProps = NULL;
    VkCheck(vkEnumerateInstanceExtensionProperties(NULL, &instExtPropCount, NULL));
    TempReserve(instExtProps, instExtPropCount);
    VkCheck(vkEnumerateInstanceExtensionProperties(NULL, &instExtPropCount, instExtProps));

    con_printf(C32_WHITE, "Vulkan Instance Extensions:");
    for (u32 i = 0; i < instExtPropCount; ++i)
    {
        con_printf(C32_WHITE, instExtProps[i].extensionName);
    }
}

static strlist_t GetExtensions(void)
{
    strlist_t list;
    strlist_new(&list, EAlloc_Temp);

    u32 glfwCount = 0;
    const char** glfwList = glfwGetRequiredInstanceExtensions(&glfwCount);
    for (u32 i = 0; i < glfwCount; ++i)
    {
        strlist_add(&list, glfwList[i]);
    }

#ifdef _DEBUG
    strlist_add(&list, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif // _DEBUG

    return list;
}

static strlist_t GetLayers(void)
{
    strlist_t list;
    strlist_new(&list, EAlloc_Temp);
#ifdef _DEBUG
    strlist_add(&list, "VK_LAYER_KHRONOS_validation");
#endif // _DEBUG
    return list;
}

static VkInstance CreateInstance(strlist_t extensions, strlist_t layers)
{
    VkInstanceCreateInfo instInfo =
    {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0x0,
        .pApplicationInfo = &kAppInfo,
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
