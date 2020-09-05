#include "rendering/vulkan/vkr_mem.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_sync.h"
#include "rendering/vulkan/vkr_device.h"
#include "rendering/vulkan/vkr_context.h"
#include "VulkanMemoryAllocator/src/vk_mem_alloc.h"
#include "allocator/allocator.h"
#include "common/profiler.h"
#include "common/time.h"
#include "threading/task.h"
#include <string.h>

#define VKR_MEM_LEAK 0

#if VKR_MEM_LEAK
typedef struct vkrLeak
{
    VmaAllocation allocation;
    const char* tag;
} vkrLeak;

static i32 ms_leakcount;
static vkrLeak* ms_leaks;

static void AddLeak(VmaAllocation allocation, const char* tag)
{
    ASSERT(allocation);
    ASSERT(tag);
    i32 leakback = ms_leakcount++;
    PermReserve(ms_leaks, leakback + 1);
    ms_leaks[leakback].allocation = allocation;
    ms_leaks[leakback].tag = tag;
}

static void RemoveLeak(VmaAllocation allocation)
{
    ASSERT(allocation);
    i32 len = ms_leakcount;
    vkrLeak* leaks = ms_leaks;
    for (i32 i = 0; i < len; ++i)
    {
        if (leaks[i].allocation == allocation)
        {
            --len;
            leaks[i] = leaks[len];
            break;
        }
    }
    ms_leakcount = len;
}
#else
#define AddLeak(allocation, tag)
#define RemoveLeak(allocation)
#endif // VKR_MEM_LEAK

ProfileMark(pm_allocfn, vkrAllocFn)
static void* VKAPI_PTR vkrAllocFn(
    void* usr,
    usize size,
    usize align,
    VkSystemAllocationScope scope)
{
    ProfileBegin(pm_allocfn);
    void* ptr = NULL;
    i32 align32 = (i32)align;
    i32 size32 = (i32)size;
    if ((align32 < 1) || (align32 > 16))
    {
        ASSERT(false);
        goto end;
    }
    if (size32 <= 0)
    {
        ASSERT(false);
        goto end;
    }
    ptr = perm_malloc(size32);
end:
    ProfileEnd(pm_allocfn);
    return ptr;
}

ProfileMark(pm_reallocfn, vkrReallocFn)
static void* VKAPI_PTR vkrReallocFn(
    void* usr,
    void* prev,
    usize size,
    usize align,
    VkSystemAllocationScope scope)
{
    ProfileBegin(pm_reallocfn);
    void* ptr = NULL;
    i32 align32 = (i32)align;
    i32 size32 = (i32)size;
    ASSERT(size32 >= 0);
    if (size32 <= 0)
    {
        pim_free(prev);
        goto end;
    }
    if ((align32 < 1) || (align32 > 16))
    {
        ASSERT(false);
        pim_free(prev);
        goto end;
    }
    ptr = perm_realloc(prev, size32);
end:
    ProfileEnd(pm_reallocfn);
    return ptr;
}

ProfileMark(pm_freefn, vkrFreeFn)
static void VKAPI_PTR vkrFreeFn(void* usr, void* prev)
{
    ProfileBegin(pm_freefn);
    pim_free(prev);
    ProfileEnd(pm_freefn);
}

static void VKAPI_PTR vkrOnAlloc(
    void* usr,
    usize size,
    VkInternalAllocationType type,
    VkSystemAllocationScope scope)
{

}

static void VKAPI_PTR vkrOnFree(
    void* usr,
    usize size,
    VkInternalAllocationType type,
    VkSystemAllocationScope scope)
{

}

static const VkAllocationCallbacks kCpuCallbacks =
{
    .pfnAllocation = vkrAllocFn,
    .pfnReallocation = vkrReallocFn,
    .pfnFree = vkrFreeFn,
    .pfnInternalAllocation = vkrOnAlloc,
    .pfnInternalFree = vkrOnFree,
};

bool vkrAllocator_New(vkrAllocator* allocator)
{
    ASSERT(allocator);
    memset(allocator, 0, sizeof(*allocator));

    const VmaVulkanFunctions vulkanFns =
    {
        .vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties,
        .vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties,
        .vkAllocateMemory = vkAllocateMemory,
        .vkFreeMemory = vkFreeMemory,
        .vkMapMemory = vkMapMemory,
        .vkUnmapMemory = vkUnmapMemory,
        .vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges,
        .vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges,
        .vkBindBufferMemory = vkBindBufferMemory,
        .vkBindImageMemory = vkBindImageMemory,
        .vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements,
        .vkGetImageMemoryRequirements = vkGetImageMemoryRequirements,
        .vkCreateBuffer = vkCreateBuffer,
        .vkDestroyBuffer = vkDestroyBuffer,
        .vkCreateImage = vkCreateImage,
        .vkDestroyImage = vkDestroyImage,
        .vkCmdCopyBuffer = vkCmdCopyBuffer,
        .vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2KHR,
        .vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2KHR,
        .vkBindBufferMemory2KHR = vkBindBufferMemory2KHR,
        .vkBindImageMemory2KHR = vkBindImageMemory2KHR,
        .vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2KHR,
    };
    const VmaAllocatorCreateInfo info =
    {
        .vulkanApiVersion = VK_API_VERSION_1_2,
        .flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT,
        .instance = g_vkr.inst,
        .physicalDevice = g_vkr.phdev,
        .device = g_vkr.dev,
        .pAllocationCallbacks = &kCpuCallbacks,
        .frameInUseCount = kFramesInFlight - 1,
        .pVulkanFunctions = &vulkanFns,
    };
    VmaAllocator handle = NULL;
    VkCheck(vmaCreateAllocator(&info, &handle));
    ASSERT(handle);

    if (handle)
    {
        allocator->handle = handle;
        mutex_create(&allocator->releasemtx);
    }

    return handle != NULL;
}

void vkrAllocator_Del(vkrAllocator* allocator)
{
    if (allocator)
    {
        if (allocator->handle)
        {
            vkrDevice_WaitIdle();
            {
                const u32 frame = time_framecount() + kFramesInFlight + 1;
                const i32 len = allocator->numreleasable;
                vkrReleasable* releasables = allocator->releasables;
                for (i32 i = 0; i < len; ++i)
                {
                    VkFence fence = releasables[i].fence;
                    if (fence)
                    {
                        vkrFence_Wait(fence);
                    }
                    if (!vkrReleasable_Del(&releasables[i], frame))
                    {
                        ASSERT(false);
                    }
                }
                pim_free(allocator->releasables);
                mutex_destroy(&allocator->releasemtx);
            }
            vmaDestroyAllocator(allocator->handle);
        }
        memset(allocator, 0, sizeof(*allocator));
    }
}

void vkrAllocator_Finalize(vkrAllocator* allocator)
{
    vkrDevice_WaitIdle();
    ASSERT(allocator);
    ASSERT(allocator->handle);
    // command fences must still be alive
    ASSERT(g_vkr.context.threadcount);

    mutex_lock(&allocator->releasemtx);
    const u32 frame = time_framecount();
    i32 len = allocator->numreleasable;
    vkrReleasable* releasables = allocator->releasables;
    for (i32 i = len - 1; i >= 0; --i)
    {
        VkFence fence = releasables[i].fence;
        if (fence)
        {
            vkrFence_Wait(fence);
            if (vkrReleasable_Del(&releasables[i], frame))
            {
                releasables[i] = releasables[len - 1];
                --len;
            }
            else
            {
                ASSERT(false);
            }
        }
    }
    allocator->numreleasable = len;
    mutex_unlock(&allocator->releasemtx);
}

ProfileMark(pm_allocupdate, vkrAllocator_Update)
void vkrAllocator_Update(vkrAllocator* allocator)
{
    ProfileBegin(pm_allocupdate);

    ASSERT(allocator);
    ASSERT(allocator->handle);
    // command fences must still be alive
    ASSERT(g_vkr.context.threadcount);

    const u32 frame = time_framecount();
    vmaSetCurrentFrameIndex(allocator->handle, frame);
    {
        mutex_lock(&allocator->releasemtx);
        i32 len = allocator->numreleasable;
        vkrReleasable* releasables = allocator->releasables;
        for (i32 i = len - 1; i >= 0; --i)
        {
            if (vkrReleasable_Del(&releasables[i], frame))
            {
                releasables[i] = releasables[len - 1];
                --len;
            }
        }
        allocator->numreleasable = len;
        mutex_unlock(&allocator->releasemtx);
    }

    ProfileEnd(pm_allocupdate);
}

ProfileMark(pm_releasableadd, vkrReleasable_Add)
void vkrReleasable_Add(vkrAllocator* allocator, const vkrReleasable* releasable)
{
    ProfileBegin(pm_releasableadd);
    ASSERT(allocator);
    ASSERT(allocator->handle);
    // command fences must still be alive
    ASSERT(g_vkr.context.threadcount);
    ASSERT(releasable);
    ASSERT(releasable->fence);
    mutex_lock(&allocator->releasemtx);
    i32 back = allocator->numreleasable++;
    PermReserve(allocator->releasables, back + 1);
    allocator->releasables[back] = *releasable;
    mutex_unlock(&allocator->releasemtx);
    ProfileEnd(pm_releasableadd);
}

ProfileMark(pm_releasabledel, vkrReleasable_Del)
bool vkrReleasable_Del(vkrReleasable* releasable, u32 frame)
{
    ProfileBegin(pm_releasabledel);
    ASSERT(releasable);
    bool ready = false;
    VkFence fence = releasable->fence;
    ASSERT(fence);
    if (fence)
    {
        vkrFenceState state = vkrFence_Stat(fence);
        ready = state != vkrFenceState_Unsignalled;
    }
    else
    {
        u32 duration = frame - releasable->frame;
        ready = duration > kFramesInFlight;
    }
    if (ready)
    {
        switch (releasable->type)
        {
        default:
            ASSERT(false);
            break;
        case vkrReleasableType_Buffer:
            vkrBuffer_Del(&releasable->buffer);
            break;
        case vkrReleasableType_Image:
            vkrImage_Del(&releasable->image);
            break;
        case vkrReleasableType_ImageView:
        {
            if (releasable->view)
            {
                vkDestroyImageView(g_vkr.dev, releasable->view, NULL);
            }
        }
        break;
        case vkrReleasableType_Sampler:
        {
            if (releasable->sampler)
            {
                vkDestroySampler(g_vkr.dev, releasable->sampler, NULL);
            }
        }
        break;
        }
        memset(releasable, 0, sizeof(*releasable));
    }
    ProfileEnd(pm_releasabledel);
    return ready;
}

VkFence vkrMem_Barrier(
    vkrQueueId id,
    VkPipelineStageFlags srcStage,
    VkPipelineStageFlags dstStage,
    const VkMemoryBarrier* glob,
    const VkBufferMemoryBarrier* buffer,
    const VkImageMemoryBarrier* img)
{
    vkrFrameContext* ctx = vkrContext_Get();
    VkCommandBuffer cmd = NULL;
    VkFence fence = NULL;
    VkQueue queue = NULL;
    vkrContext_GetCmd(ctx, id, &cmd, &fence, &queue);
    vkrCmdBegin(cmd);
    {
        vkCmdPipelineBarrier(
            cmd,
            srcStage, dstStage, 0x0,
            glob ? 1 : 0, glob,
            buffer ? 1 : 0, buffer,
            img ? 1 : 0, img);
    }
    vkrCmdEnd(cmd);
    vkrCmdSubmit(queue, cmd, fence, NULL, 0x0, NULL);
    ASSERT(fence);
    return fence;
}

const VkAllocationCallbacks* vkrMem_Fns(void)
{
    return &kCpuCallbacks;
}

ProfileMark(pm_memmap, vkrMem_Map)
void* vkrMem_Map(VmaAllocation allocation)
{
    ProfileBegin(pm_memmap);
    void* result = NULL;
    ASSERT(allocation);
    VkCheck(vmaMapMemory(g_vkr.allocator.handle, allocation, &result));
    ASSERT(result);
    ProfileEnd(pm_memmap);
    return result;
}

ProfileMark(pm_memunmap, vkrMem_Unmap)
void vkrMem_Unmap(VmaAllocation allocation)
{
    ProfileBegin(pm_memunmap);
    ASSERT(allocation);
    vmaUnmapMemory(g_vkr.allocator.handle, allocation);
    ProfileEnd(pm_memunmap);
}

ProfileMark(pm_memflush, vkrMem_Flush)
void vkrMem_Flush(VmaAllocation allocation)
{
    ProfileBegin(pm_memflush);
    ASSERT(allocation);
    const VkDeviceSize offset = 0;
    const VkDeviceSize size = VK_WHOLE_SIZE;
    VkCheck(vmaFlushAllocation(g_vkr.allocator.handle, allocation, offset, size));
    VkCheck(vmaInvalidateAllocation(g_vkr.allocator.handle, allocation, offset, size));
    ProfileEnd(pm_memflush);
}

ProfileMark(pm_bufnew, vkrBuffer_New)
bool vkrBuffer_New(
    vkrBuffer* buffer,
    i32 size,
    VkBufferUsageFlags bufferUsage,
    vkrMemUsage memUsage,
    const char* tag)
{
    ProfileBegin(pm_bufnew);
    ASSERT(g_vkr.allocator.handle);
    ASSERT(size >= 0);
    ASSERT(buffer);
    memset(buffer, 0, sizeof(*buffer));
    const VkBufferCreateInfo bufferInfo =
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = bufferUsage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    const VmaAllocationCreateInfo allocInfo =
    {
        .usage = memUsage,
    };
    VkBuffer handle = NULL;
    VmaAllocation allocation = NULL;
    VkCheck(vmaCreateBuffer(
        g_vkr.allocator.handle,
        &bufferInfo,
        &allocInfo,
        &handle,
        &allocation,
        NULL));
    ASSERT(handle);
    ProfileEnd(pm_bufnew);
    if (handle)
    {
        buffer->handle = handle;
        buffer->allocation = allocation;
        buffer->size = size;
        AddLeak(allocation, tag);
    }
    return handle != NULL;
}

ProfileMark(pm_bufdel, vkrBuffer_Del)
void vkrBuffer_Del(vkrBuffer* buffer)
{
    ProfileBegin(pm_bufdel);
    ASSERT(g_vkr.allocator.handle);
    if (buffer)
    {
        if (buffer->handle)
        {
            vmaDestroyBuffer(
                g_vkr.allocator.handle,
                buffer->handle,
                buffer->allocation);
            RemoveLeak(buffer->allocation);
        }
        memset(buffer, 0, sizeof(*buffer));
    }
    ProfileEnd(pm_bufdel);
}

ProfileMark(pm_bufreserve, vkrBuffer_Reserve)
bool vkrBuffer_Reserve(
    vkrBuffer* buffer,
    i32 size,
    VkBufferUsageFlags bufferUsage,
    vkrMemUsage memUsage,
    VkFence fence,
    const char* tag)
{
    ProfileBegin(pm_bufreserve);
    bool success = true;
    i32 oldsize = buffer->size;
    ASSERT(buffer);
    ASSERT(size >= 0);
    ASSERT(oldsize >= 0);
    if (oldsize < size)
    {
        vkrBuffer_Release(buffer, fence);
        size = (size > oldsize * 2) ? size : oldsize * 2;
        if (!vkrBuffer_New(buffer, size, bufferUsage, memUsage, tag))
        {
            success = false;
            goto cleanup;
        }
    }
cleanup:
    ProfileEnd(pm_bufreserve);
    return success;
}

void* vkrBuffer_Map(const vkrBuffer* buffer)
{
    ASSERT(buffer);
    return vkrMem_Map(buffer->allocation);
}

void vkrBuffer_Unmap(const vkrBuffer* buffer)
{
    ASSERT(buffer);
    vkrMem_Unmap(buffer->allocation);
}

void vkrBuffer_Flush(const vkrBuffer* buffer)
{
    ASSERT(buffer);
    vkrMem_Flush(buffer->allocation);
}

ProfileMark(pm_bufferwrite, vkrBuffer_Write)
void vkrBuffer_Write(const vkrBuffer* buffer, const void* src, i32 size)
{
    ProfileBegin(pm_bufferwrite);
    ASSERT(buffer);
    ASSERT(src);
    ASSERT(size <= buffer->size);
    ASSERT(size >= 0);
    void* dst = vkrBuffer_Map(buffer);
    if (dst)
    {
        memcpy(dst, src, size);
    }
    vkrBuffer_Unmap(buffer);
    vkrBuffer_Flush(buffer);
    ProfileEnd(pm_bufferwrite);
}

ProfileMark(pm_bufrelease, vkrBuffer_Release)
void vkrBuffer_Release(vkrBuffer* buffer, VkFence fence)
{
    ProfileBegin(pm_bufrelease);
    ASSERT(buffer);
    ASSERT(fence);
    if (buffer->handle)
    {
        const vkrReleasable releasable =
        {
            .frame = time_framecount(),
            .type = vkrReleasableType_Buffer,
            .fence = fence,
            .buffer = *buffer,
        };
        vkrReleasable_Add(&g_vkr.allocator, &releasable);
    }
    memset(buffer, 0, sizeof(*buffer));
    ProfileEnd(pm_bufrelease);
}

ProfileMark(pm_imgnew, vkrImage_New)
bool vkrImage_New(
    vkrImage* image,
    const VkImageCreateInfo* info,
    vkrMemUsage memUsage,
    const char* tag)
{
    ProfileBegin(pm_imgnew);
    memset(image, 0, sizeof(*image));
    const VmaAllocationCreateInfo allocInfo =
    {
        .usage = memUsage,
    };
    VkImage handle = NULL;
    VmaAllocation allocation = NULL;
    VkCheck(vmaCreateImage(
        g_vkr.allocator.handle,
        info,
        &allocInfo,
        &handle,
        &allocation,
        NULL));
    ProfileEnd(pm_imgnew);
    if (handle)
    {
        AddLeak(allocation, tag);
        image->handle = handle;
        image->allocation = allocation;
        AddLeak(allocation, PIM_FILELINE());
        return true;
    }
    return false;
}

ProfileMark(pm_imgdel, vkrImage_Del)
void vkrImage_Del(vkrImage* image)
{
    ProfileBegin(pm_imgdel);
    if (image)
    {
        if (image->handle)
        {
            RemoveLeak(image->allocation);
            vmaDestroyImage(
                g_vkr.allocator.handle,
                image->handle,
                image->allocation);
        }
        memset(image, 0, sizeof(*image));
    }
    ProfileEnd(pm_imgdel);
}

void* vkrImage_Map(const vkrImage* image)
{
    ASSERT(image);
    return vkrMem_Map(image->allocation);
}

void vkrImage_Unmap(const vkrImage* image)
{
    ASSERT(image);
    vkrMem_Unmap(image->allocation);
}

void vkrImage_Flush(const vkrImage* image)
{
    ASSERT(image);
    vkrMem_Flush(image->allocation);
}

ProfileMark(pm_imgrelease, vkrImage_Release)
void vkrImage_Release(vkrImage* image, VkFence fence)
{
    ProfileBegin(pm_imgrelease);
    ASSERT(image);
    if (image->handle)
    {
        const vkrReleasable releasable =
        {
            .frame = time_framecount(),
            .type = vkrReleasableType_Image,
            .fence = fence,
            .image = *image,
        };
        vkrReleasable_Add(&g_vkr.allocator, &releasable);
    }
    memset(image, 0, sizeof(*image));
    ProfileEnd(pm_imgrelease);
}
