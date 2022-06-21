#pragma once

#include "common/macro.h"
#include "rendering/r_config.h"
#include <volk/volk.h>
#include "math/types.h"

PIM_C_BEGIN

#define VkCheck(expr) do { VkResult _res = (expr); ASSERT(_res == VK_SUCCESS); } while(0)

typedef struct GLFWwindow GLFWwindow;
typedef struct VmaAllocation_T* VmaAllocation;

typedef enum
{
    vkrQueueId_Graphics,
    vkrQueueId_Compute,
    vkrQueueId_Transfer,
    vkrQueueId_Present,

    vkrQueueId_COUNT
} vkrQueueId;

typedef enum
{
    vkrQueueFlag_GraphicsBit = 1 << vkrQueueId_Graphics,
    vkrQueueFlag_ComputeBit = 1 << vkrQueueId_Compute,
    vkrQueueFlag_TransferBit = 1 << vkrQueueId_Transfer,
    vkrQueueFlag_PresentBit = 1 << vkrQueueId_Present,

    vkrQueueFlag_ALL = 0x7fffffff,
} vkrQueueFlagBits;
typedef u32 vkrQueueFlags;

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
    vkrMemUsage_Dynamic = 3,    // mappable, 1 host write, cached device reads
    vkrMemUsage_Readback = 4,   // mappable, 1 device write, cached host reads
} vkrMemUsage;

typedef struct vkrSubmitId_s
{
    u32 counter;
    u32 queueId : 4;
    u32 valid : 1;
} vkrSubmitId;

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

typedef struct vkrSubImageState_s
{
    VkPipelineStageFlags stage;
    VkAccessFlags access;
    VkImageLayout layout;
} vkrSubImageState_t;

typedef struct vkrImgState_s
{
    vkrQueueId owner;
    u32 cmdId;
    VkPipelineStageFlags stage;
    VkAccessFlags access;
    VkImageLayout layout;
    vkrSubImageState_t* substates;
} vkrImageState_t;

typedef struct vkrBufferState_s
{
    vkrQueueId owner;
    u32 cmdId;
    VkPipelineStageFlags stage;
    VkAccessFlags access;
} vkrBufferState_t;

typedef struct vkrBuffer_s
{
    vkrBufferState_t state;
    VkBuffer handle;
    VmaAllocation allocation;
    i32 size;
} vkrBuffer;

typedef struct vkrBufferSet_s
{
    vkrBuffer frames[R_ResourceSets];
} vkrBufferSet;

typedef struct vkrImage_s
{
    vkrImageState_t state;
    VkImage handle;
    VmaAllocation allocation;
    VkImageView view;
    VkFormat format;
    u32 width : 16;
    u32 height : 16;
    u32 depth : 12;
    u32 mipLevels : 8;
    u32 arrayLayers : 8;
    u32 usage : 8; // VkImageUsageFlagBits
    u32 type : 2; // VkImageType
    u32 imported : 1; // true if from swapchain
} vkrImage;

typedef struct vkrImageSet_s
{
    vkrImage frames[R_ResourceSets];
} vkrImageSet;

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
    u32 colorWriteMask : 4; // VkColorComponentFlags
    u32 blendEnable : 1;
    u32 srcColorBlendFactor : 5; // VkBlendFactor
    u32 dstColorBlendFactor : 5; // VkBlendFactor
    u32 colorBlendOp : 3; // VkBlendOp
    u32 srcAlphaBlendFactor : 5; // VkBlendFactor
    u32 dstAlphaBlendFactor : 5; // VkBlendFactor
    u32 alphaBlendOp : 3; // VkBlendOp
} vkrBlendState;

typedef struct vkrFixedFuncs_s
{
    VkViewport viewport;
    VkRect2D scissor;
    u32 topology : 3; // VkPrimitiveTopology
    u32 polygonMode : 2; // VkPolygonMode
    u32 cullMode : 2; // VkCullModeFlagBits
    u32 frontFace : 1; // VkFrontFace
    u32 depthCompareOp : 3; // VkCompareOp
    u32 scissorOn : 1;
    u32 depthClamp : 1;
    u32 depthTestEnable : 1;
    u32 depthWriteEnable : 1;
    u32 attachmentCount : 3;
    vkrBlendState attachments[8];
} vkrFixedFuncs;

typedef struct vkrWindow_s
{
    GLFWwindow* handle;
    VkSurfaceKHR surface;
    i32 width;
    i32 height;
    bool fullscreen;
} vkrWindow;

typedef struct vkrTargets_s
{
    i32 width;
    i32 height;
    vkrImage depth[R_ResourceSets];
    vkrImage scene[R_ResourceSets];
} vkrTargets;

typedef struct vkrSwapchain_s
{
    VkSwapchainKHR handle;
    VkFormat colorFormat;
    VkColorSpaceKHR colorSpace;
    VkPresentModeKHR mode;
    i32 width;
    i32 height;

    i32 length;
    u32 imageIndex;
    vkrSubmitId imageSubmits[R_MaxSwapchainLen];
    vkrImage images[R_MaxSwapchainLen];

    u32 syncIndex;
    vkrSubmitId syncSubmits[R_ResourceSets];
    VkSemaphore availableSemas[R_ResourceSets];
    VkSemaphore renderedSemas[R_ResourceSets];
} vkrSwapchain;

typedef struct vkrQueue_s
{
    VkQueue handle;
    i32 family;
    i32 index;
    VkAccessFlags accessMask;
    VkPipelineStageFlags stageMask;
    u32 queueId : 4;
    u32 gfx : 1;
    u32 comp : 1;
    u32 xfer : 1;
    u32 pres : 1;

    VkCommandPool cmdPool;
    VkCommandBuffer cmds[R_CmdsPerQueue];
    VkFence cmdFences[R_CmdsPerQueue];
    u32 cmdIds[R_CmdsPerQueue];
    u32 head;
    u32 tail;
} vkrQueue;

typedef struct vkrCmdBuf_s
{
    VkCommandBuffer handle;
    VkFence fence;
    u32 id;
    u32 queueId : 4; // vkrQueueId
    // indicates which types of cmds are legal on this cmdbuf
    u32 gfx : 1;
    u32 comp : 1;
    u32 xfer : 1;
    u32 pres : 1;
    // cmd state
    u32 began : 1;
    u32 ended : 1;
    u32 submit : 1;
    u32 inRenderPass : 1;
    u32 subpass : 8;
    u32 queueTransferSrc : 1;
    u32 queueTransferDst : 1;
} vkrCmdBuf;

typedef struct vkrContext_s
{
    vkrCmdBuf curCmdBuf[vkrQueueId_COUNT];
    vkrCmdBuf prevCmdBuf[vkrQueueId_COUNT];
    vkrQueueId lastSubmitQueue;
    vkrQueueId mostRecentBegin;
} vkrContext;

typedef enum
{
    vkrReleasableType_Buffer,
    vkrReleasableType_Image,
    vkrReleasableType_ImageView,
    vkrReleasableType_Attachment, // view used as an attachment
} vkrReleasableType;

typedef struct vkrReleasable_s
{
    //u32 frame;              // frame that resource was released
    vkrSubmitId submitId;
    vkrReleasableType type; // type of resource
    union
    {
        struct vkrReleasableBuffer_s
        {
            VkBuffer handle;
            VmaAllocation allocation;
        } buffer;
        struct vkrReleasableImage_s
        {
            VkImage handle;
            VmaAllocation allocation;
            VkImageView view;
        } image;
        VkImageView view;
    };
} vkrReleasable;

// https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#renderpass-compatibility
// compatibility:
// refs: matching format and sample count, or both VK_ATTACHMENT_UNUSED, or both NULL
// arrays of refs: treats missing refs as VK_ATTACHMENT_UNUSED

typedef struct vkrAttachmentState_s
{
    VkFormat format;        // must match
    VkImageLayout layout;   // can vary
    u8 load : 2;            // VkAttachmentLoadOp, can vary
    u8 store : 2;           // VkAttachmentStoreOp, can vary
} vkrAttachmentState;

typedef struct vkrRenderPassDesc_s
{
    vkrAttachmentState attachments[8];
    VkAccessFlags srcAccessMask;
    VkAccessFlags dstAccessMask;
    VkPipelineStageFlags srcStageMask;
    VkPipelineStageFlags dstStageMask;
} vkrRenderPassDesc;

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
    fn(KHR_acceleration_structure); \
    fn(KHR_ray_tracing_pipeline); \
    fn(KHR_ray_query); \
    fn(KHR_deferred_host_operations); \
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

    vkrWindow window;
    vkrSwapchain chain;
    vkrQueue queues[vkrQueueId_COUNT];

    vkrContext context;
} vkrSys;
extern vkrSys g_vkr;

bool vkrSys_Init(void);
bool vkrSys_WindowUpdate(void);
void vkrSys_Update(void);
void vkrSys_Shutdown(void);

// frame in flight index
u32 vkrGetSyncIndex(void);
u32 vkrGetPrevSyncIndex(void);
u32 vkrGetNextSyncIndex(void);
// swapchain image index
u32 vkrGetSwapIndex(void);
// frame count
u32 vkrGetFrameCount(void);

vkrContext* vkrGetContext(void);
vkrQueue* vkrGetQueue(vkrQueueId queueId);

bool vkrGetHdrEnabled(void);
float vkrGetWhitepoint(void);
float vkrGetDisplayNitsMin(void);
float vkrGetDisplayNitsMax(void);
float vkrGetUiNits(void);
Colorspace vkrGetRenderColorspace(void);
Colorspace vkrGetDisplayColorspace(void);

// vkr_targets.c
i32 vkrGetRenderWidth(void);
i32 vkrGetRenderHeight(void);
float vkrGetRenderAspect(void);
float vkrGetRenderScale(void);
i32 vkrGetDisplayWidth(void);
i32 vkrGetDisplayHeight(void);
vkrImage* vkrGetBackBuffer(void);
vkrImage* vkrGetDepthBuffer(void);
vkrImage* vkrGetSceneBuffer(void);
vkrImage* vkrGetPrevSceneBuffer(void);

PIM_C_END
