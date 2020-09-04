#pragma once

#include "common/macro.h"
#include <volk/volk.h>
#include "math/types.h"
#include "containers/strlist.h"
#include "threading/mutex.h"

PIM_C_BEGIN

#define VkCheck(expr) do { VkResult _res = (expr); ASSERT(_res == VK_SUCCESS); } while(0)

#define kMaxSwapchainLen        4
#define kFramesInFlight         3
#define kDrawsPerThread         128
#define kCamerasPerThread       1

PIM_FWD_DECL(GLFWwindow);
PIM_DECL_HANDLE(VmaAllocator);
PIM_DECL_HANDLE(VmaAllocation);

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

typedef enum
{
    vkrMeshStream_Position, // float4; xyz=positionOS
    vkrMeshStream_Normal,   // float4; xyz=normalOS
    vkrMeshStream_Uv01,     // float4; xy=uv0, zw=uv1

    vkrMeshStream_COUNT
} vkrMeshStream;

// https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/vkGetFenceStatus.html
typedef enum
{
    vkrFenceState_Signalled = VK_SUCCESS,
    vkrFenceState_Unsignalled = VK_NOT_READY,
    vkrFenceState_Lost = VK_ERROR_DEVICE_LOST,
} vkrFenceState;

typedef struct vkrBuffer
{
    VkBuffer handle;
    VmaAllocation allocation;
    i32 size;
} vkrBuffer;

typedef struct vkrImage
{
    VkImage handle;
    VmaAllocation allocation;
} vkrImage;

typedef struct vkrMesh
{
    vkrBuffer buffer;
    i32 vertCount;
    i32 indexCount;
} vkrMesh;

typedef struct vkrTexture2D
{
    vkrImage image;
    i32 width;
    i32 height;
} vkrTexture2D;

typedef struct vkrBufferBinding
{
    vkrBuffer buffer;
    VkDescriptorSet set;
    i32 binding;
    VkDescriptorType type;
} vkrBufferBinding;

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
    VkPipelineLayout handle;
    VkDescriptorSetLayout* sets;
    VkPushConstantRange* ranges;
    i32 setCount;
    i32 rangeCount;
} vkrPipelineLayout;

typedef struct vkrPipeline
{
    VkPipeline handle;
    VkRenderPass renderPass;
    vkrPipelineLayout layout;
    VkPipelineBindPoint bindpoint;
    i32 subpass;
} vkrPipeline;

typedef struct vkrDisplay
{
    GLFWwindow* window;
    VkSurfaceKHR surface;
    i32 width;
    i32 height;
} vkrDisplay;

typedef struct vkrSwapchain
{
    VkSwapchainKHR handle;
    VkFormat format;
    VkColorSpaceKHR colorSpace;
    VkPresentModeKHR mode;
    i32 width;
    i32 height;

    i32 length;
    u32 imageIndex;
    VkImage images[kMaxSwapchainLen];
    VkImageView views[kMaxSwapchainLen];
    VkFramebuffer buffers[kMaxSwapchainLen];
    VkFence imageFences[kMaxSwapchainLen];

    u32 syncIndex;
    VkFence syncFences[kFramesInFlight];
    VkSemaphore availableSemas[kFramesInFlight];
    VkSemaphore renderedSemas[kFramesInFlight];

} vkrSwapchain;

typedef struct vkrQueue
{
    VkQueue handle;
    i32 family;
    i32 index;
    VkExtent3D granularity;
} vkrQueue;

typedef struct vkrCmdAlloc
{
    VkCommandPool pool;
    VkQueue queue;
    VkCommandBufferLevel level;
    u32 frame;
    u32 head;
    u32 capacity;
    VkCommandBuffer* buffers;
    VkFence* fences;
} vkrCmdAlloc;

typedef struct vkrFrameContext
{
    vkrCmdAlloc cmds[vkrQueueId_COUNT];     // primary level cmd buffers
    vkrCmdAlloc seccmds[vkrQueueId_COUNT];  // secondary level cmd buffers
    VkDescriptorPool descpool;
} vkrFrameContext;

typedef struct vkrThreadContext
{
    vkrFrameContext frames[kFramesInFlight];
} vkrThreadContext;

typedef struct vkrContext
{
    i32 threadcount;
    vkrThreadContext* threads;
    vkrBuffer perdrawbuf;
    vkrBuffer percambuf;
} vkrContext;

typedef enum
{
    vkrReleasableType_Buffer,
    vkrReleasableType_Image,
} vkrReleasableType;

typedef struct vkrReleasable
{
    u32 frame;              // frame that resource was released
    vkrReleasableType type; // type of resource
    VkFence fence;          // optional, used in place of frame
    union
    {
        vkrBuffer buffer;
        vkrImage image;
    };
} vkrReleasable;

typedef struct vkrAllocator
{
    VmaAllocator handle;
    vkrReleasable* releasables;
    i32 numreleasable;
    mutex_t releasemtx;
} vkrAllocator;

typedef struct vkrPerDraw
{
    float4x4 localToWorld;
    float4x4 worldToLocal;
    float4 textureScale;
    float4 textureBias;
} vkrPerDraw;

typedef struct vkrPerCamera
{
    float4x4 worldToCamera;
    float4x4 cameraToClip;
} vkrPerCamera;

typedef struct vkrPushConstants
{
    u32 drawIndex;
    u32 cameraIndex;
} vkrPushConstants;

// ----------------------------------------------------------------------------

typedef struct vkr_t
{
    VkInstance inst;
    VkPhysicalDevice phdev;
    VkDevice dev;
    vkrAllocator allocator;
    VkPhysicalDeviceFeatures phdevFeats;
    VkPhysicalDeviceProperties phdevProps;
    VkDebugUtilsMessengerEXT messenger;

    vkrDisplay display;
    vkrSwapchain chain;
    vkrQueue queues[vkrQueueId_COUNT];

    vkrContext context;

    vkrPipeline pipeline;

} vkr_t;
extern vkr_t g_vkr;

bool vkr_init(i32 width, i32 height);
void vkr_update(void);
void vkr_shutdown(void);

PIM_C_END
