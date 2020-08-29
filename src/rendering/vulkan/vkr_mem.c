#include "rendering/vulkan/vkr_mem.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_sync.h"
#include "VulkanMemoryAllocator/src/vk_mem_alloc.h"
#include "allocator/allocator.h"
#include "common/profiler.h"
#include "threading/task.h"
#include <string.h>

static void* VKAPI_PTR vkrAllocFn(
    void* usr,
    usize size,
    usize align,
    VkSystemAllocationScope scope)
{
    i32 align32 = (i32)align;
    i32 size32 = (i32)size;
    if ((align32 < 1) || (align32 > 16))
    {
        ASSERT(false);
        return NULL;
    }
    if (size32 <= 0)
    {
        ASSERT(false);
        return NULL;
    }
    return perm_malloc(size32);
}

static void* VKAPI_PTR vkrReallocFn(
    void* usr,
    void* prev,
    usize size,
    usize align,
    VkSystemAllocationScope scope)
{
    i32 align32 = (i32)align;
    i32 size32 = (i32)size;
    ASSERT(size32 >= 0);
    if (size32 <= 0)
    {
        pim_free(prev);
        return NULL;
    }
    if ((align32 < 1) || (align32 > 16))
    {
        ASSERT(false);
        pim_free(prev);
        return NULL;
    }
    return perm_realloc(prev, size32);
}

static void VKAPI_PTR vkrFreeFn(void* usr, void* prev)
{
    pim_free(prev);
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

bool vkrMem_Init(vkr_t* vkr)
{
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
        .instance = vkr->inst,
        .physicalDevice = vkr->phdev,
        .device = vkr->dev,
        .pAllocationCallbacks = &kCpuCallbacks,
        .frameInUseCount = kFramesInFlight - 1,
        .pVulkanFunctions = &vulkanFns,
    };
    VmaAllocator handle = NULL;
    VkCheck(vmaCreateAllocator(&info, &handle));
    ASSERT(handle);
    vkr->allocator = handle;
    return handle != NULL;
}

void vkrMem_Shutdown(vkr_t* vkr)
{
    if (vkr->allocator)
    {
        vmaDestroyAllocator(vkr->allocator);
        vkr->allocator = NULL;
    }
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
    VkCheck(vmaMapMemory(g_vkr.allocator, allocation, &result));
    ASSERT(result);
    ProfileEnd(pm_memmap);
    return result;
}

ProfileMark(pm_memunmap, vkrMem_Unmap)
void vkrMem_Unmap(VmaAllocation allocation)
{
    ProfileBegin(pm_memunmap);
    ASSERT(allocation);
    vmaUnmapMemory(g_vkr.allocator, allocation);
    ProfileEnd(pm_memunmap);
}

ProfileMark(pm_memflush, vkrMem_Flush)
void vkrMem_Flush(VmaAllocation allocation)
{
    ProfileBegin(pm_memflush);
    ASSERT(allocation);
    const VkDeviceSize offset = 0;
    const VkDeviceSize size = VK_WHOLE_SIZE;
    VkCheck(vmaFlushAllocation(g_vkr.allocator, allocation, offset, size));
    VkCheck(vmaInvalidateAllocation(g_vkr.allocator, allocation, offset, size));
    ProfileEnd(pm_memflush);
}

ProfileMark(pm_bufnew, vkrBuffer_New)
bool vkrBuffer_New(
    vkrBuffer* buffer,
    i32 size,
    VkBufferUsageFlags bufferUsage,
    vkrMemUsage memUsage)
{
    ProfileBegin(pm_bufnew);
    ASSERT(g_vkr.allocator);
    ASSERT(size >= 0);
    ASSERT(buffer);
    memset(buffer, 0, sizeof(*buffer));
    const VkBufferCreateInfo bufferInfo =
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = bufferUsage,
    };
    const VmaAllocationCreateInfo allocInfo =
    {
        .usage = memUsage,
    };
    VkBuffer handle = NULL;
    VmaAllocation allocation = NULL;
    VkCheck(vmaCreateBuffer(
        g_vkr.allocator,
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
        return true;
    }
    return false;
}

ProfileMark(pm_bufdel, vkrBuffer_Del)
void vkrBuffer_Del(vkrBuffer* buffer)
{
    ProfileBegin(pm_bufdel);
    ASSERT(g_vkr.allocator);
    if (buffer)
    {
        if (buffer->handle)
        {
            vmaDestroyBuffer(g_vkr.allocator, buffer->handle, buffer->allocation);
        }
        memset(buffer, 0, sizeof(*buffer));
    }
    ProfileEnd(pm_bufdel);
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

ProfileMark(pm_imgnew, vkrImage_New)
bool vkrImage_New(
    vkrImage* image,
    const VkImageCreateInfo* info,
    vkrMemUsage memUsage)
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
        g_vkr.allocator,
        info,
        &allocInfo,
        &handle,
        &allocation,
        NULL));
    ProfileEnd(pm_imgnew);
    if (handle)
    {
        image->handle = handle;
        image->allocation = allocation;
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
            vmaDestroyImage(g_vkr.allocator, image->handle, image->allocation);
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
