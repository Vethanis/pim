#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

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

bool vkrMem_Init(vkr_t* vkr);
void vkrMem_Shutdown(vkr_t* vkr);

const VkAllocationCallbacks* vkrMem_Fns(void);

bool vkrBuffer_New(
    vkrBuffer* buffer,
    i32 size,
    VkBufferUsageFlags bufferUsage,
    u32 memTypeBits,
    vkrMemUsage memUsage);
void vkrBuffer_Del(vkrBuffer* buffer);

bool vkrImage_New(
    vkrImage* image,
    const VkImageCreateInfo* info,
    u32 memTypeBits,
    vkrMemUsage memUsage);
void vkrImage_Del(vkrImage* image);

PIM_C_END
