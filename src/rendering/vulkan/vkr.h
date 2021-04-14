#pragma once

#include "common/macro.h"
#include <volk/volk.h>
#include "math/types.h"
#include "threading/spinlock.h"

PIM_C_BEGIN

#define VkCheck(expr) do { VkResult _res = (expr); ASSERT(_res == VK_SUCCESS); } while(0)

PIM_FWD_DECL(GLFWwindow);
PIM_DECL_HANDLE(VmaAllocation);

enum
{
    kMaxSwapchainLen = 3,
    kDesiredSwapchainLen = 3,
    kFramesInFlight = 3,

    kHistogramSize = 256,

    kTextureTable1DSize = 64,
    kTextureTable2DSize = 512,
    kTextureTable3DSize = 64,
    kTextureTableCubeSize = 64,
    kTextureTable2DArraySize = 64,
};

// single descriptors
typedef enum
{
    // uniforms
    vkrBindId_CameraData,

    // storage images
    vkrBindId_LumTexture,

    // storage buffers
    vkrBindId_HistogramBuffer,
    vkrBindId_ExposureBuffer,

    vkrBindId_COUNT
} vkrBindId;

// descriptor arrays
typedef enum
{
    vkrBindTableId_Texture1D = vkrBindId_COUNT,
    vkrBindTableId_Texture2D,
    vkrBindTableId_Texture3D,
    vkrBindTableId_TextureCube,
    vkrBindTableId_Texture2DArray,
} vkrBindTableId;

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
    vkrMeshStream_Position,     // float4; xyz=positionOS
    vkrMeshStream_Normal,       // float4; xyz=normalOS, w=uv1 image index
    vkrMeshStream_Uv01,         // float4; xy=uv0, zw=uv1
    vkrMeshStream_TexIndices,   // int4; albedo, rome, normal, lightmap index

    vkrMeshStream_COUNT
} vkrMeshStream;

// https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/vkGetFenceStatus.html
typedef enum
{
    vkrFenceState_Signalled = VK_SUCCESS,
    vkrFenceState_Unsignalled = VK_NOT_READY,
    vkrFenceState_Lost = VK_ERROR_DEVICE_LOST,
} vkrFenceState;

// Mirror of VmaMemoryUsage enum to avoid exposing giant header
typedef enum
{
    vkrMemUsage_Unknown = 0,
    vkrMemUsage_GpuOnly = 1,
    vkrMemUsage_CpuOnly = 2,
    vkrMemUsage_CpuToGpu = 3,
    vkrMemUsage_GpuToCpu = 4,
    vkrMemUsage_CpuCopy = 5,
} vkrMemUsage;

typedef enum
{
    vkrPassId_Depth,
    vkrPassId_Opaque,
    vkrPassId_UI,

    vkrPassId_COUNT
} vkrPassId;

typedef struct vkrTextureId_s
{
    u32 version : 8;
    u32 type : 3; // VkImageViewType
    u32 index : 21;
} vkrTextureId;
SASSERT(sizeof(vkrTextureId) == 4);

typedef struct vkrMeshId_s
{
    u32 version : 8;
    u32 index : 24;
} vkrMeshId;
SASSERT(sizeof(vkrMeshId) == 4);

typedef struct vkrBuffer_s
{
    VkBuffer handle;
    VmaAllocation allocation;
    i32 size;
} vkrBuffer;

typedef struct vkrBufferSet_s
{
    vkrBuffer frames[kFramesInFlight];
} vkrBufferSet;

typedef struct vkrImage_s
{
    VkImage handle;
    VmaAllocation allocation;
    VkImageView view;
    VkImageType type;
    VkFormat format;
    VkImageLayout layout;
    VkImageUsageFlags usage;
    u16 width;
    u16 height;
    u16 depth;
    u8 mipLevels;
    u8 arrayLayers;
} vkrImage;

typedef struct vkrImageSet_s
{
    vkrImage frames[kFramesInFlight];
} vkrImageSet;

typedef struct vkrFramebuffer_s
{
    VkFramebuffer handle;
    VkImageView attachments[8];
    VkFormat formats[8];
    i32 width;
    i32 height;
} vkrFramebuffer;

typedef struct vkrCompileInput_s
{
    const char* filename;
    const char* entrypoint;
    const char* text;
    const char** macroKeys;
    const char** macroValues;
    i32 macroCount;
    vkrShaderType type;
    bool compile;
    bool disassemble;
} vkrCompileInput;

typedef struct vkrCompileOutput_s
{
    u32* dwords;
    i32 dwordCount;
    char* errors;
    char* disassembly;
} vkrCompileOutput;

typedef struct vkrQueueSupport_s
{
    i32 family[vkrQueueId_COUNT];
    i32 index[vkrQueueId_COUNT];
    u32 count;
    VkQueueFamilyProperties* properties;
} vkrQueueSupport;

typedef struct vkrSwapchainSupport_s
{
    VkSurfaceCapabilitiesKHR caps;
    u32 formatCount;
    VkSurfaceFormatKHR* formats;
    u32 modeCount;
    VkPresentModeKHR* modes;
} vkrSwapchainSupport;

typedef struct vkrVertexLayout_s
{
    i32 bindingCount;
    const VkVertexInputBindingDescription* bindings;
    i32 attributeCount;
    const VkVertexInputAttributeDescription* attributes;
} vkrVertexLayout;

typedef struct vkrBlendState_s
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

typedef struct vkrFixedFuncs_s
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

typedef struct vkrDisplay_s
{
    GLFWwindow* window;
    VkSurfaceKHR surface;
    i32 width;
    i32 height;
} vkrDisplay;

typedef struct vkrSwapchain_s
{
    VkSwapchainKHR handle;
    VkRenderPass presentPass;
    VkFormat colorFormat;
    VkColorSpaceKHR colorSpace;
    VkPresentModeKHR mode;
    i32 width;
    i32 height;

    i32 length;
    u32 imageIndex;
    VkFence imageFences[kMaxSwapchainLen];
    VkImage images[kMaxSwapchainLen];
    VkImageView views[kMaxSwapchainLen];
    vkrFramebuffer buffers[kMaxSwapchainLen];
    vkrImage lumAttachments[kMaxSwapchainLen];
    vkrImage depthAttachments[kMaxSwapchainLen];

    u32 syncIndex;
    VkFence syncFences[kFramesInFlight];
    VkSemaphore availableSemas[kFramesInFlight];
    VkSemaphore renderedSemas[kFramesInFlight];
    VkCommandBuffer presCmds[kFramesInFlight];
    VkCommandPool cmdpool;

} vkrSwapchain;

typedef struct vkrQueue_s
{
    VkQueue handle;
    i32 family;
    i32 index;
    VkExtent3D granularity;
} vkrQueue;

typedef struct vkrCmdAlloc_s
{
    VkCommandPool pool;
    VkQueue queue;
    VkCommandBufferLevel level;
    u32 head;
    u32 capacity;
    VkCommandBuffer* buffers;
    VkFence* fences;
} vkrCmdAlloc;

typedef struct vkrThreadContext_s
{
    vkrCmdAlloc cmds[vkrQueueId_COUNT];     // primary level cmd buffers
    vkrCmdAlloc seccmds[vkrQueueId_COUNT];  // secondary level cmd buffers
} vkrThreadContext;

typedef struct vkrContext_s
{
    i32 threadcount;
    vkrThreadContext* threads;
} vkrContext;

typedef enum
{
    vkrReleasableType_Buffer,
    vkrReleasableType_Image,
    vkrReleasableType_ImageView,
} vkrReleasableType;

typedef struct vkrReleasable_s
{
    u32 frame;              // frame that resource was released
    vkrReleasableType type; // type of resource
    union
    {
        vkrBuffer buffer;
        vkrImage image;
        VkImageView view;
    };
} vkrReleasable;

// https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#renderpass-compatibility
// compatibility:
// refs: matching format and sample count, or both VK_ATTACHMENT_UNUSED, or both NULL
// arrays of refs: treats missing refs as VK_ATTACHMENT_UNUSED

typedef struct vkrAttachmentState_s
{
    VkFormat format;                // must match
    VkImageLayout initialLayout;    // can vary
    VkImageLayout layout;           // can vary
    VkImageLayout finalLayout;      // can vary
    VkAttachmentLoadOp load;        // can vary
    VkAttachmentStoreOp store;      // can vary
} vkrAttachmentState;

typedef struct vkrRenderPassDesc_s
{
    vkrAttachmentState attachments[8];
    VkAccessFlags srcAccessMask;
    VkAccessFlags dstAccessMask;
    VkPipelineStageFlags srcStageMask;
    VkPipelineStageFlags dstStageMask;
} vkrRenderPassDesc;

typedef struct vkrPassContext_s
{
    VkFramebuffer framebuffer;
    VkCommandBuffer cmd;
    VkFence fence;
} vkrPassContext;

typedef struct vkrPassDesc_s
{
// Graphics and Compute
    i32 pushConstantBytes;
    i32 shaderCount;
    const VkPipelineShaderStageCreateInfo* shaders;
// Graphics only
    VkRenderPass renderPass;
    i32 subpass;
    vkrVertexLayout vertLayout;
    vkrFixedFuncs fixedFuncs;
} vkrPassDesc;

typedef struct vkrPass_s
{
    VkPipeline pipeline;
    VkPipelineLayout layout;
    VkPipelineBindPoint bindpoint;
    VkShaderStageFlagBits stageFlags;
    i32 pushConstantBytes;
} vkrPass;

// ----------------------------------------------------------------------------

typedef struct vkrExposure_s
{
    float exposure;
    float avgLum;
    float deltaTime;
    float adaptRate;

    float aperture;
    float shutterTime;
    float ISO;

    // offsets the output exposure, in EV
    float offsetEV;
    // EV range to consider
    float minEV;
    float maxEV;
    // range of the cdf to consider
    float minCdf;
    float maxCdf;

    // manual: ev100 is based on camera parameters
    // automatic: ev100 is based on luminance
    i32 manual;
    // standard output or saturation based exposure
    i32 standard;
} vkrExposure;

// ----------------------------------------------------------------------------

typedef struct vkrProps_s
{
    VkPhysicalDeviceProperties2 phdev;
    VkPhysicalDeviceRayTracingPropertiesNV rtnv;
    VkPhysicalDeviceAccelerationStructurePropertiesKHR accstr;
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtpipe;
} vkrProps;
extern vkrProps g_vkrProps;

typedef struct vkrFeats_s
{
    VkPhysicalDeviceFeatures2 phdev;
    VkPhysicalDeviceAccelerationStructureFeaturesKHR accstr;
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtpipe;
    VkPhysicalDeviceRayQueryFeaturesKHR rquery;
} vkrFeats;
extern vkrFeats g_vkrFeats;

typedef struct vkrDevExts_s
{
#define VKR_DEV_EXTS(fn) \
    fn(KHR_swapchain); \
    fn(NV_ray_tracing); \
    fn(KHR_acceleration_structure); \
    fn(KHR_ray_tracing_pipeline); \
    fn(KHR_ray_query); \
    fn(EXT_memory_budget); \
    fn(EXT_hdr_metadata); \
    fn(KHR_shader_float16_int8); \
    fn(KHR_16bit_storage); \
    fn(KHR_push_descriptor); \
    fn(EXT_memory_priority); \
    fn(KHR_bind_memory2); \
    fn(KHR_shader_float_controls); \
    fn(KHR_spirv_1_4); \
    fn(EXT_conditional_rendering); \
    fn(KHR_draw_indirect_count);
#define VKR_FN(name) u32 name : 1
    VKR_DEV_EXTS(VKR_FN)
#undef VKR_FN
} vkrDevExts;
extern vkrDevExts g_vkrDevExts;

typedef struct vkrInstExts_s
{
#define VKR_INST_EXTS(fn) \
    fn(KHR_get_physical_device_properties2); \
    fn(EXT_swapchain_colorspace); \
    VKR_DEBUG_MESSENGER_ONLY(fn(EXT_debug_utils);)
#define VKR_FN(name) u32 name : 1
    VKR_INST_EXTS(VKR_FN)
#undef VKR_FN
} vkrInstExts;
extern vkrInstExts g_vkrInstExts;

typedef struct vkrLayers_s
{
    u32 _empty : 1;
#define VKR_LAYERS(fn) \
    VKR_KHRONOS_LAYER_ONLY(fn(KHRONOS_validation);) \
    VKR_ASSIST_LAYER_ONLY(fn(LUNARG_assistant_layer);)
#define VKR_FN(name) u32 name : 1
    VKR_LAYERS(VKR_FN)
#undef VKR_FN
} vkrLayers;
extern vkrLayers g_vkrLayers;

typedef struct vkrSys_s
{
    VkInstance inst;
    VkPhysicalDevice phdev;
    VkDevice dev;
    VkDebugUtilsMessengerEXT messenger;

    vkrDisplay display;
    vkrSwapchain chain;
    vkrQueue queues[vkrQueueId_COUNT];

    vkrContext context;
} vkrSys;
extern vkrSys g_vkr;

bool vkrSys_Init(void);
void vkrSys_Update(void);
void vkrSys_Shutdown(void);

void vkrSys_OnLoad(void);
void vkrSys_OnUnload(void);

// frame in flight index
u32 vkrSys_SyncIndex(void);
// swapchain image index
u32 vkrSys_SwapIndex(void);
// frame count
u32 vkrSys_FrameIndex(void);

bool vkrSys_HdrEnabled(void);
float vkrSys_GetWhitepoint(void);

PIM_C_END
