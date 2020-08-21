#include "rendering/vulkan/vkr_debug.h"
#include "common/console.h"
#include <string.h>

VkDebugUtilsMessengerEXT vkrCreateDebugMessenger()
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

void vkrDestroyDebugMessenger(VkDebugUtilsMessengerEXT messenger)
{
#ifdef _DEBUG
    if (messenger)
    {
        vkDestroyDebugUtilsMessengerEXT(g_vkr.inst, messenger, NULL);
    }
#endif // _DEBUG
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
