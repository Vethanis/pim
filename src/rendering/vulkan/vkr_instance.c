#include "rendering/vulkan/vkr_instance.h"
#include "rendering/vulkan/vkr_debug.h"
#include "allocator/allocator.h"
#include "common/console.h"
#include "common/stringutil.h"
#include <string.h>
#include <GLFW/glfw3.h>

bool vkrInstance_Init(vkr_t* vkr)
{
    ASSERT(vkr);

    VkCheck(volkInitialize());

    vkrListInstExtensions();

    vkr->inst = vkrCreateInstance(vkrGetInstExtensions(), vkrGetLayers());
    ASSERT(vkr->inst);
    if (!vkr->inst)
    {
        return false;
    }

    volkLoadInstance(vkr->inst);

    vkr->messenger = vkrCreateDebugMessenger();

    return true;
}

void vkrInstance_Shutdown(vkr_t* vkr)
{
    if (vkr)
    {
        vkrDestroyDebugMessenger(vkr->messenger);
        vkr->messenger = NULL;
        if (vkr->inst)
        {
            vkDestroyInstance(vkr->inst, NULL);
            vkr->inst = NULL;
        }
    }
}

// ----------------------------------------------------------------------------

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

    vkrTryAddExtension(&list, props, count, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

#ifdef _DEBUG
    // https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VK_EXT_debug_utils.html
    vkrTryAddExtension(&list, props, count, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif // _DEBUG

    return list;
}

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
