#pragma once

#include "common/macro.h"
#include <volk/volk.h>
#include "containers/strlist.h"

PIM_C_BEGIN

PIM_FWD_DECL(GLFWwindow);

#define VkCheck(expr) do { VkResult _res = (expr); ASSERT(_res == VK_SUCCESS); } while(0)

typedef enum
{
    vkrQueueId_Pres,
    vkrQueueId_Gfx,
    vkrQueueId_Comp,
    vkrQueueId_Xfer,

    vkrQueueId_COUNT
} vkrQueueId;

typedef struct vkr_t
{
    VkInstance inst;
    VkPhysicalDevice phdev;
    VkDevice dev;
    VkQueue queues[vkrQueueId_COUNT];
    VkQueueFamilyProperties queueProps[vkrQueueId_COUNT];
    VkPhysicalDeviceFeatures phdevFeats;
    VkPhysicalDeviceProperties phdevProps;
    VkDebugUtilsMessengerEXT messenger;
} vkr_t;
extern vkr_t g_vkr;

typedef struct vkrDisplay
{
    GLFWwindow* win;
    VkSurfaceKHR surf;
    VkSwapchainKHR swap;
    u32 imgCount;
    VkImage* images;
    VkImageView* views;
    i32 format;
    i32 colorSpace;
    i32 width;
    i32 height;
} vkrDisplay;
extern vkrDisplay g_vkrdisp;

void vkr_init(i32 width, i32 height);
void vkr_update(void);
void vkr_shutdown(void);

// ----------------------------------------------------------------------------

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
    char* errors;
    char* disassembly;
} vkrCompileOutput;

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
    VkCullModeFlags cullMode;
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
} vkrRenderPass;

typedef struct vkrPipeline
{
    i32 refcount;
    VkPipeline handle;
    vkrPipelineLayout* layout;
    vkrRenderPass* renderPass;
    i32 subpass;
} vkrPipeline;

PIM_C_END
