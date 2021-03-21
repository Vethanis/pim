#pragma once

#include "common/macro.h"
#include <volk/volk.h>
#include "math/types.h"
#include "containers/strlist.h"
#include "containers/queue.h"
#include "threading/spinlock.h"

PIM_C_BEGIN

#define VkCheck(expr) do { VkResult _res = (expr); ASSERT(_res == VK_SUCCESS); } while(0)

PIM_FWD_DECL(GLFWwindow);
PIM_DECL_HANDLE(VmaAllocator);
PIM_DECL_HANDLE(VmaAllocation);
PIM_DECL_HANDLE(VmaPool);

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

typedef struct vkrMesh_s
{
    vkrBuffer buffer;
    i32 vertCount;
} vkrMesh;

typedef struct vkrImage_s
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

typedef struct vkrImageSet_s
{
    vkrImage frames[kFramesInFlight];
} vkrImageSet;

typedef struct vkrAttachment_s
{
    vkrImage image;
    VkImageView view;
} vkrAttachment;

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

typedef struct vkrAllocator_s
{
    spinlock_t lock;
    VmaAllocator handle;
    VmaPool stagePool;
    VmaPool texturePool;
    VmaPool gpuMeshPool;
    VmaPool cpuMeshPool;
    VmaPool uavPool;
    vkrReleasable* releasables;
    i32 numreleasable;
} vkrAllocator;

typedef struct vkrPassContext_s
{
    VkRenderPass renderPass;
    i32 subpass;
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
} vkrPass;

// ----------------------------------------------------------------------------

typedef struct vkrScreenBlit_s
{
    vkrBuffer meshbuf;
    vkrBuffer stagebuf;
    vkrImage image;
    i32 width;
    i32 height;
} vkrScreenBlit;

typedef struct vkrDepthPass_s
{
    vkrPass pass;
    float4x4 worldToClip;
} vkrDepthPass;

typedef struct vkrPerCamera_s
{
    float4x4 worldToClip;
    float4 eye;
    float exposure;
} vkrPerCamera;

typedef struct vkrOpaquePc_s
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

typedef struct vkrOpaquePass_s
{
    vkrPass pass;
    vkrBuffer perCameraBuffer[kFramesInFlight];
} vkrOpaquePass;

typedef struct vkrUIPassPc_s
{
    float2 scale;
    float2 translate;
    u32 textureIndex;
    u32 discardAlpha;
} vkrUIPassPc;

typedef struct vkrUIPass_s
{
    vkrPass pass;
    vkrBuffer vertbufs[kFramesInFlight];
    vkrBuffer indbufs[kFramesInFlight];
    vkrTextureId font;
} vkrUIPass;

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

typedef struct vkrExposureConstants_s
{
    u32 width;
    u32 height;
    vkrExposure exposure;
} vkrExposureConstants;

typedef struct vkrExposurePass_s
{
    vkrPass pass;
    VkPipeline adapt;
    vkrBuffer histBuffers[kFramesInFlight];
    vkrBuffer expBuffers[kFramesInFlight];
    vkrExposure params;
} vkrExposurePass;

typedef struct vkrMainPass_s
{
    VkRenderPass renderPass;
    vkrScreenBlit blit;
    vkrDepthPass depth;
    vkrOpaquePass opaque;
    vkrUIPass ui;
} vkrMainPass;

// ----------------------------------------------------------------------------

typedef struct vkrSys_s
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

PIM_C_END
