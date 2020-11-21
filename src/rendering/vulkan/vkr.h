#pragma once

#include "common/macro.h"
#include <volk/volk.h>
#include "math/types.h"
#include "containers/strlist.h"
#include "containers/queue.h"
#include "threading/mutex.h"

PIM_C_BEGIN

#define VkCheck(expr) do { VkResult _res = (expr); ASSERT(_res == VK_SUCCESS); } while(0)

PIM_FWD_DECL(GLFWwindow);
PIM_DECL_HANDLE(VmaAllocator);
PIM_DECL_HANDLE(VmaAllocation);

typedef enum
{
    kMaxSwapchainLen = 3,
    kDesiredSwapchainLen = 3,
    kFramesInFlight = 3,
    kTextureDescriptors = 1024,
    kHistogramSize = 256,
} vkrLimits;

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
    // No intended memory usage specified.
    //Use other members of VmaAllocationCreateInfo to specify your requirements.
    vkrMemUsage_Unknown = 0,

    // Memory will be used on device only, so fast access from the device is preferred.
    //It usually means device-local GPU (video) memory.
    //No need to be mappable on host.
    //It is roughly equivalent of `D3D12_HEAP_TYPE_DEFAULT`.
    //Usage:
    //- Resources written and read by device, e.g. images used as attachments.
    //- Resources transferred from host once (immutable) or infrequently and read by
    //  device multiple times, e.g. textures to be sampled, vertex buffers, uniform
    //  (constant) buffers, and majority of other types of resources used on GPU.
    //Allocation may still end up in `HOST_VISIBLE` memory on some implementations.
    //In such case, you are free to map it.
    //You can use #VMA_ALLOCATION_CREATE_MAPPED_BIT with this usage type.
    vkrMemUsage_GpuOnly = 1,

    // Memory will be mappable on host.
    //It usually means CPU (system) memory.
    //Guarantees to be `HOST_VISIBLE` and `HOST_COHERENT`.
    //CPU access is typically uncached. Writes may be write-combined.
    //Resources created in this pool may still be accessible to the device, but access to them can be slow.
    //It is roughly equivalent of `D3D12_HEAP_TYPE_UPLOAD`.
    //Usage: Staging copy of resources used as transfer source.
    vkrMemUsage_CpuOnly = 2,

    //Memory that is both mappable on host (guarantees to be `HOST_VISIBLE`) and preferably fast to access by GPU.
    //CPU access is typically uncached. Writes may be write-combined.
    //Usage: Resources written frequently by host (dynamic), read by device. E.g. textures (with LINEAR layout), vertex buffers, uniform buffers updated every frame or every draw call.
    vkrMemUsage_CpuToGpu = 3,

    // Memory mappable on host (guarantees to be `HOST_VISIBLE`) and cached.
    //It is roughly equivalent of `D3D12_HEAP_TYPE_READBACK`.
    //Usage:
    //- Resources written by device, read by host - results of some computations, e.g. screen capture, average scene luminance for HDR tone mapping.
    //- Any resources read or accessed randomly on host, e.g. CPU-side copy of vertex buffer used as source of transfer, but also used for collision detection.
    vkrMemUsage_GpuToCpu = 4,

    //* CPU memory - memory that is preferably not `DEVICE_LOCAL`, but also not guaranteed to be `HOST_VISIBLE`.
    //Usage: Staging copy of resources moved from GPU memory to CPU memory as part
    //of custom paging/residency mechanism, to be moved back to GPU memory when needed.
    vkrMemUsage_CpuCopy = 5,
} vkrMemUsage;

typedef enum
{
    vkrPassId_Depth,
    vkrPassId_Opaque,
    vkrPassId_UI,

    vkrPassId_COUNT
} vkrPassId;

typedef struct vkrBuffer
{
    VkBuffer handle;
    VmaAllocation allocation;
    i32 size;
} vkrBuffer;

typedef struct vkrMesh
{
    vkrBuffer buffer;
    i32 vertCount;
} vkrMesh;

typedef union vkrTextureId
{
    struct
    {
        u32 index : 24;
        u32 version : 8;
    };
    u32 asint;
} vkrTextureId;

typedef struct vkrImage
{
    VkImage handle;
    VmaAllocation allocation;
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

typedef struct vkrTexture2D
{
    vkrImage image;
    VkSampler sampler;
    VkImageView view;
} vkrTexture2D;

typedef struct vkrAttachment
{
    vkrImage image;
    VkImageView view;
} vkrAttachment;

typedef struct vkrBinding
{
    VkDescriptorSet set;
    union
    {
        VkDescriptorBufferInfo buffer;
        VkDescriptorImageInfo image;
    };
    VkDescriptorType type;
    u16 arrayElem;
    u8 binding;
} vkrBinding;

typedef struct vkrCompileInput
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

typedef struct vkrCompileOutput
{
    u32* dwords;
    i32 dwordCount;
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

typedef struct vkrVertexLayout
{
    i32 bindingCount;
    const VkVertexInputBindingDescription* bindings;
    i32 attributeCount;
    const VkVertexInputAttributeDescription* attributes;
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
    VkFormat colorFormat;
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
    vkrAttachment lumAttachments[kMaxSwapchainLen];
    vkrAttachment depthAttachments[kMaxSwapchainLen];

    u32 syncIndex;
    VkFence syncFences[kFramesInFlight];
    VkSemaphore availableSemas[kFramesInFlight];
    VkSemaphore renderedSemas[kFramesInFlight];
    VkCommandBuffer presCmds[kFramesInFlight];
    VkCommandPool cmdpool;

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
    u32 head;
    u32 capacity;
    VkCommandBuffer* buffers;
    VkFence* fences;
} vkrCmdAlloc;

typedef struct vkrThreadContext
{
    vkrCmdAlloc cmds[vkrQueueId_COUNT];     // primary level cmd buffers
    vkrCmdAlloc seccmds[vkrQueueId_COUNT];  // secondary level cmd buffers
} vkrThreadContext;

typedef struct vkrContext
{
    i32 threadcount;
    vkrThreadContext* threads;
} vkrContext;

typedef enum
{
    vkrReleasableType_Buffer,
    vkrReleasableType_Image,
    vkrReleasableType_ImageView,
    vkrReleasableType_Sampler,
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
        VkImageView view;
        VkSampler sampler;
    };
} vkrReleasable;

typedef struct vkrAllocator
{
    VmaAllocator handle;
    vkrReleasable* releasables;
    i32 numreleasable;
    mutex_t releasemtx;
} vkrAllocator;

typedef struct vkrPassContext
{
    VkRenderPass renderPass;
    i32 subpass;
    u32 syncIndex;
    u32 imageIndex;
    VkFramebuffer framebuffer;
    VkCommandBuffer cmd;
    VkFence fence;
} vkrPassContext;

typedef struct vkrPassDesc
{
// Graphics and Compute
    VkPipelineBindPoint bindpoint;
    i32 poolSizeCount;
    const VkDescriptorPoolSize* poolSizes;
    i32 bindingCount;
    const VkDescriptorSetLayoutBinding* bindings;
    i32 rangeCount;
    const VkPushConstantRange* ranges;
    i32 shaderCount;
    const VkPipelineShaderStageCreateInfo* shaders;
// Graphics only
    VkRenderPass renderPass;
    i32 subpass;
    vkrVertexLayout vertLayout;
    vkrFixedFuncs fixedFuncs;
} vkrPassDesc;

typedef struct vkrPass
{
    VkPipeline pipeline;
    VkPipelineLayout layout;
    VkDescriptorSetLayout setLayout;
    VkDescriptorPool descPool;
    VkDescriptorSet sets[kFramesInFlight];
} vkrPass;

// ----------------------------------------------------------------------------

typedef struct vkrScreenBlit
{
    vkrBuffer meshbuf;
    vkrBuffer stagebuf;
    vkrImage image;
    i32 width;
    i32 height;
} vkrScreenBlit;

typedef struct vkrDepthPass
{
    vkrPass pass;
    vkrBuffer stagebufs[kFramesInFlight];
    vkrBuffer meshbufs[kFramesInFlight];
    i32 vertCount;
} vkrDepthPass;

typedef struct vkrPerCamera
{
    float4x4 worldToClip;
    float4 eye;
    u32 lmBegin;
    float exposure;
} vkrPerCamera;

typedef struct vkrOpaquePc
{
    float4x4 localToWorld;
    float4 IMc0;
    float4 IMc1;
    float4 IMc2;
    float4 flatRome;
    u32 albedoIndex;
    u32 romeIndex;
    u32 normalIndex;
} vkrOpaquePc;

typedef struct vkrOpaquePass
{
    vkrPass pass;
    vkrBuffer perCameraBuffer[kFramesInFlight];
} vkrOpaquePass;

typedef struct vkrUIPassPc
{
    float2 scale;
    float2 translate;
    u32 textureIndex;
    u32 discardAlpha;
} vkrUIPassPc;

typedef struct vkrUIPass
{
    vkrPass pass;
    vkrBuffer vertbufs[kFramesInFlight];
    vkrBuffer indbufs[kFramesInFlight];
    vkrTextureId font;
} vkrUIPass;

typedef struct vkrExposure
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

typedef struct vkrExposureConstants
{
    u32 width;
    u32 height;
    vkrExposure exposure;
} vkrExposureConstants;

typedef struct vkrExposurePass
{
    vkrPass pass;
    VkPipeline adapt;
    vkrBuffer histBuffers[kFramesInFlight];
    vkrBuffer expBuffers[kFramesInFlight];
    vkrExposure params;
} vkrExposurePass;

typedef struct vkrMainPass
{
    VkRenderPass renderPass;
    vkrScreenBlit blit;
    vkrDepthPass depth;
    vkrOpaquePass opaque;
    vkrUIPass ui;
} vkrMainPass;

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

    vkrMainPass mainPass;
    vkrExposurePass exposurePass;
} vkr_t;
extern vkr_t g_vkr;

bool vkr_init(void);
void vkr_update(void);
void vkr_shutdown(void);

void vkr_onload(void);
void vkr_onunload(void);

PIM_C_END
