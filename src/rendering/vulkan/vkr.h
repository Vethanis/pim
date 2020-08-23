#pragma once

#include "common/macro.h"
#include <volk/volk.h>
#include "containers/strlist.h"

PIM_C_BEGIN

#define VkCheck(expr) do { VkResult _res = (expr); ASSERT(_res == VK_SUCCESS); } while(0)

#define kFramesInFlight         3
#define vkrAlive(ptr)           ((ptr) && ((ptr)->refcount > 0))

typedef struct GLFWwindow GLFWwindow;

typedef enum
{
    vkrQueueId_Pres,
    vkrQueueId_Gfx,
    vkrQueueId_Comp,
    vkrQueueId_Xfer,

    vkrQueueId_COUNT
} vkrQueueId;

typedef enum
{
    vkrShaderType_Vert,
    vkrShaderType_Frag,
    vkrShaderType_Comp,
    vkrShaderType_Raygen,
    vkrShaderType_AnyHit,
    vkrShaderType_ClosestHit,
    vkrShaderType_Miss,
    vkrShaderType_Isect,
    vkrShaderType_Call,
    vkrShaderType_Task,
    vkrShaderType_Mesh,
} vkrShaderType;

typedef enum
{
    vkrVertType_float,
    vkrVertType_float2,
    vkrVertType_float3,
    vkrVertType_float4,
    vkrVertType_int,
    vkrVertType_int2,
    vkrVertType_int3,
    vkrVertType_int4,
} vkrVertType;

typedef struct vkrCompileInput
{
    char* filename;
    char* entrypoint;
    char* text;
    char** macroKeys;
    char** macroValues;
    i32 macroCount;
    vkrShaderType type;
    bool compile;
    bool disassemble;
} vkrCompileInput;

typedef struct vkrCompileOutput
{
    vkrShaderType type;
    u32* dwords;
    i32 dwordCount;
    char* entrypoint;
    char* errors;
    char* disassembly;
} vkrCompileOutput;

typedef struct vkrQueueSupport
{
    i32 family[vkrQueueId_COUNT];
    i32 index[vkrQueueId_COUNT];
    u32 count;
    VkQueueFamilyProperties* properties;
} vkrQueueSupport;

typedef struct vkrSwapchainSupport
{
    VkSurfaceCapabilitiesKHR caps;
    u32 formatCount;
    VkSurfaceFormatKHR* formats;
    u32 modeCount;
    VkPresentModeKHR* modes;
} vkrSwapchainSupport;

// vkr supports 'SoA' layout (1 binding per attribute, no interleaving)
// with a maximum of 8 streams, in sequential location
typedef struct vkrVertexLayout
{
    i32 streamCount;
    vkrVertType types[8];
} vkrVertexLayout;

typedef struct vkrBlendState
{
    VkColorComponentFlags colorWriteMask;
    bool blendEnable;
    VkBlendFactor srcColorBlendFactor;
    VkBlendFactor dstColorBlendFactor;
    VkBlendOp colorBlendOp;
    VkBlendFactor srcAlphaBlendFactor;
    VkBlendFactor dstAlphaBlendFactor;
    VkBlendOp alphaBlendOp;
} vkrBlendState;

typedef struct vkrFixedFuncs
{
    VkViewport viewport;
    VkRect2D scissor;
    VkPrimitiveTopology topology;
    VkPolygonMode polygonMode;
    VkCullModeFlagBits cullMode;
    VkFrontFace frontFace;
    VkCompareOp depthCompareOp;
    bool scissorOn;
    bool depthClamp;
    bool depthTestEnable;
    bool depthWriteEnable;
    i32 attachmentCount;
    vkrBlendState attachments[8];
} vkrFixedFuncs;

typedef struct vkrPipelineLayout
{
    i32 refcount;
    VkPipelineLayout handle;
    VkDescriptorSetLayout* sets;
    VkPushConstantRange* ranges;
    i32 setCount;
    i32 rangeCount;
} vkrPipelineLayout;

typedef struct vkrRenderPass
{
    i32 refcount;
    VkRenderPass handle;
    i32 subpassCount;
    i32 attachmentCount;
    i32 dependencyCount;
} vkrRenderPass;

typedef struct vkrPipeline
{
    i32 refcount;
    VkPipeline handle;
    VkPipelineBindPoint bindpoint;
    vkrPipelineLayout* layout;
    vkrRenderPass* renderPass;
    i32 subpass;
} vkrPipeline;

typedef struct vkrDisplay
{
    i32 refcount;
    GLFWwindow* window;
    VkSurfaceKHR surface;
    i32 width;
    i32 height;
} vkrDisplay;

typedef struct vkrSwapFrame
{
    VkImage image;
    VkImageView view;
    VkFramebuffer buffer;
    VkFence fence; // copy of fences[i]
} vkrSwapFrame;

typedef struct vkrSwapchain
{
    i32 refcount;
    VkSwapchainKHR handle;
    vkrDisplay* display;
    VkFormat format;
    VkColorSpaceKHR colorSpace;
    VkPresentModeKHR mode;
    i32 width;
    i32 height;
    i32 length;
    vkrSwapFrame* frames;
    u32 syncIndex;
    u32 imageIndex;
    VkFence fences[kFramesInFlight];
    VkSemaphore availableSemas[kFramesInFlight];
    VkSemaphore renderedSemas[kFramesInFlight];
} vkrSwapchain;

typedef struct vkrQueue
{
    VkQueue handle;
    i32 family;
    i32 index;
    i32 threadcount;
    VkCommandPool* pools[kFramesInFlight];
    VkCommandBuffer* buffers[kFramesInFlight];
    VkExtent3D granularity;
} vkrQueue;

typedef struct vkrCmdBuf
{
    VkCommandBuffer handle;
    vkrQueue* queue;
    i32 frame;
    i32 tid;
} vkrCmdBuf;

// ----------------------------------------------------------------------------

typedef struct vkr_t
{
    VkInstance inst;
    VkPhysicalDevice phdev;
    VkDevice dev;
    VkPhysicalDeviceFeatures phdevFeats;
    VkPhysicalDeviceProperties phdevProps;
    VkDebugUtilsMessengerEXT messenger;

    vkrDisplay* display;
    vkrSwapchain* chain;
    vkrQueue queues[vkrQueueId_COUNT];

    vkrPipeline* pipeline;
    vkrRenderPass* renderpass;
    vkrPipelineLayout* layout;
} vkr_t;
extern vkr_t g_vkr;

bool vkr_init(i32 width, i32 height);
void vkr_update(void);
void vkr_shutdown(void);

PIM_C_END
