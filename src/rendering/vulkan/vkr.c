#include "rendering/vulkan/vkr.h"
#include "rendering/vulkan/vkrcompiler.h"
#include "rendering/vulkan/vkr_debug.h"
#include "rendering/vulkan/vkr_device.h"
#include "rendering/vulkan/vkr_swapchain.h"
#include <GLFW/glfw3.h>
#include "allocator/allocator.h"
#include "common/console.h"
#include "common/stringutil.h"
#include <string.h>

vkr_t g_vkr;
vkrDisplay g_vkrdisp;

static VKAPI_ATTR VkBool32 VKAPI_CALL vkrOnVulkanMessage(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData);

void vkr_init(i32 width, i32 height)
{
    memset(&g_vkr, 0, sizeof(g_vkr));
    memset(&g_vkrdisp, 0, sizeof(g_vkrdisp));

    VkCheck(volkInitialize());

    i32 rv = glfwInit();
    ASSERT(rv == GLFW_TRUE);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    g_vkrdisp.win = glfwCreateWindow(width, height, "pimvk", NULL, NULL);
    ASSERT(g_vkrdisp.win);
    g_vkrdisp.width = width;
    g_vkrdisp.height = height;

    vkrListInstExtensions();
    g_vkr.inst = vkrCreateInstance(vkrGetInstExtensions(), vkrGetLayers());
    ASSERT(g_vkr.inst);

    volkLoadInstance(g_vkr.inst);

    g_vkr.messenger = vkrCreateDebugMessenger();

    g_vkrdisp.surf = vkrCreateSurface(g_vkr.inst, g_vkrdisp.win);
    ASSERT(g_vkrdisp.surf);

    g_vkr.phdev = vkrSelectPhysicalDevice(&g_vkr.phdevProps, &g_vkr.phdevFeats);
    ASSERT(g_vkr.phdev);

    vkrListDevExtensions(g_vkr.phdev);

    g_vkr.dev = vkrCreateDevice(
        vkrGetDevExtensions(g_vkr.phdev),
        vkrGetLayers(),
        g_vkr.queues,
        g_vkr.queueProps);
    ASSERT(g_vkr.dev);

    volkLoadDevice(g_vkr.dev);

    vkrCreateSwapchain(&g_vkrdisp, NULL);
    ASSERT(g_vkrdisp.swap);

    const vkrCompileInput input =
    {
        .filename = "example.hlsl",
        .entrypoint = "main",
        .text = "struct PSInput{float4 color : COLOR; }; float4 main(PSInput input) : SV_TARGET { return input.color; }",
        .type = vkrShaderType_Frag,
        .compile = true,
        .disassemble = true,
    };
    vkrCompileOutput output = { 0 };
    vkrCompile(&input, &output);
    if (output.errors)
    {
        con_logf(LogSev_Error, "Vkc", "%s", output.errors);
    }
    if (output.disassembly)
    {
        con_logf(LogSev_Info, "Vkc", "%s", output.disassembly);
    }
    vkrCompileOutput_Del(&output);
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

    vkrDestroyDebugMessenger(&g_vkr.messenger);

    vkDestroyInstance(g_vkr.inst, NULL);
    g_vkr.inst = NULL;

    glfwDestroyWindow(g_vkrdisp.win);
    g_vkrdisp.win = NULL;
}

