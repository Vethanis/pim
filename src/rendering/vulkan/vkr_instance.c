#include "rendering/vulkan/vkr_instance.h"
#include "rendering/vulkan/vkr_debug.h"
#include "rendering/vulkan/vkr_extension.h"
#include "allocator/allocator.h"
#include "common/console.h"
#include "common/stringutil.h"
#include <string.h>
#include <GLFW/glfw3.h>

bool vkrInstance_Init(vkrSys* vkr)
{
    ASSERT(vkr);

    VkCheck(volkInitialize());

    vkrListInstLayers();
    vkrListInstExtensions();

    vkr->inst = vkrCreateInstance(
        vkrGetInstExtensions(&g_vkrInstExts),
        vkrGetLayers(&g_vkrLayers));
    ASSERT(vkr->inst);
    if (!vkr->inst)
    {
        return false;
    }

    volkLoadInstance(vkr->inst);

    vkr->messenger = vkrCreateDebugMessenger();

    return true;
}

void vkrInstance_Shutdown(vkrSys* vkr)
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

VkInstance vkrCreateInstance(StrList extensions, StrList layers)
{
    const VkApplicationInfo appInfo =
    {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "pim",
        .applicationVersion = 1,
        .pEngineName = "pim",
        .engineVersion = 1,
        .apiVersion = VK_API_VERSION_1_2,
    };

    const VkInstanceCreateInfo instInfo =
    {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .flags = 0x0,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = layers.count,
        .ppEnabledLayerNames = layers.ptr,
        .enabledExtensionCount = extensions.count,
        .ppEnabledExtensionNames = extensions.ptr,
    };

    VkInstance inst = NULL;
    VkCheck(vkCreateInstance(&instInfo, NULL, &inst));

    StrList_Del(&extensions);
    StrList_Del(&layers);

    return inst;
}
