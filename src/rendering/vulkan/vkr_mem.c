#include "rendering/vulkan/vkr_mem.h"

#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_sync.h"
#include "rendering/vulkan/vkr_device.h"
#include "rendering/vulkan/vkr_context.h"

#include "allocator/allocator.h"
#include "common/profiler.h"
#include "common/time.h"
#include "common/console.h"
#include "common/cvars.h"
#include "threading/task.h"

#include "VulkanMemoryAllocator/src/vk_mem_alloc.h"
#include <string.h>

typedef struct vkrAllocator_s
{
    Spinlock lock;
    VmaAllocator handle;
    VmaPool stagePool;
    VmaPool texturePool;
    VmaPool gpuMeshPool;
    VmaPool cpuMeshPool;
    VmaPool uavPool;
    vkrReleasable* releasables;
    i32 numreleasable;
} vkrAllocator;

static vkrAllocator ms_inst;

static VmaPool GetBufferPool(VkBufferUsageFlags usage, vkrMemUsage memUsage);
static VmaPool GetTexturePool(VkImageUsageFlags usage, vkrMemUsage memUsage);
static void FinalizeCheck(i32 len);

bool vkrMemSys_Init(void)
{
    bool success = true;

    vkrAllocator *const allocator = &ms_inst;
    memset(allocator, 0, sizeof(*allocator));
    Spinlock_New(&allocator->lock);

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
            .flags =
                VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT |
                VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT,
            .instance = g_vkr.inst,
            .physicalDevice = g_vkr.phdev,
            .device = g_vkr.dev,
            .pAllocationCallbacks = NULL,
            .frameInUseCount = kFramesInFlight - 1,
            .pVulkanFunctions = &vulkanFns,
        };
        VmaAllocator handle = NULL;
        VkCheck(vmaCreateAllocator(&info, &handle));
        ASSERT(handle);
        allocator->handle = handle;
        if (!handle)
        {
            success = false;
            goto cleanup;
        }
    }

    {
        const VkBufferCreateInfo bufferInfo =
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = sizeof(float4) * 4 * 1024,
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };
        const VmaAllocationCreateInfo allocInfo =
        {
            .flags = VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT,
            .usage = vkrMemUsage_CpuOnly,
        };
        u32 memTypeIndex = 0;
        VkCheck(vmaFindMemoryTypeIndexForBufferInfo(
            allocator->handle, &bufferInfo, &allocInfo, &memTypeIndex));
        const VmaPoolCreateInfo poolInfo =
        {
            .memoryTypeIndex = memTypeIndex,
            .frameInUseCount = kFramesInFlight - 1,
        };
        VmaPool pool = NULL;
        VkCheck(vmaCreatePool(allocator->handle, &poolInfo, &pool));
        ASSERT(pool);
        allocator->stagePool = pool;
        if (!pool)
        {
            success = false;
            goto cleanup;
        }
    }

    {
        const VkBufferCreateInfo bufferInfo =
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = sizeof(float4) * 4 * 1024,
            .usage =
                VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };
        const VmaAllocationCreateInfo allocInfo =
        {
            .flags = VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT,
            .usage = vkrMemUsage_GpuOnly,
        };
        u32 memTypeIndex = 0;
        VkCheck(vmaFindMemoryTypeIndexForBufferInfo(
            allocator->handle, &bufferInfo, &allocInfo, &memTypeIndex));
        const VmaPoolCreateInfo poolInfo =
        {
            .memoryTypeIndex = memTypeIndex,
            .frameInUseCount = kFramesInFlight - 1,
        };
        VmaPool pool = NULL;
        VkCheck(vmaCreatePool(allocator->handle, &poolInfo, &pool));
        ASSERT(pool);
        allocator->gpuMeshPool = pool;
        if (!pool)
        {
            success = false;
            goto cleanup;
        }
    }

    {
        const VkBufferCreateInfo bufferInfo =
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = sizeof(float4) * 4 * 1024,
            .usage =
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };
        const VmaAllocationCreateInfo allocInfo =
        {
            .flags = VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT,
            .usage = vkrMemUsage_CpuToGpu,
        };
        u32 memTypeIndex = 0;
        VkCheck(vmaFindMemoryTypeIndexForBufferInfo(
            allocator->handle, &bufferInfo, &allocInfo, &memTypeIndex));
        const VmaPoolCreateInfo poolInfo =
        {
            .memoryTypeIndex = memTypeIndex,
            .frameInUseCount = kFramesInFlight - 1,
        };
        VmaPool pool = NULL;
        VkCheck(vmaCreatePool(allocator->handle, &poolInfo, &pool));
        ASSERT(pool);
        allocator->cpuMeshPool = pool;
        if (!pool)
        {
            success = false;
            goto cleanup;
        }
    }

    {
        const VkBufferCreateInfo bufferInfo =
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = sizeof(float4) * 4 * 1024,
            .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };
        const VmaAllocationCreateInfo allocInfo =
        {
            .flags = VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT,
            .usage = vkrMemUsage_CpuToGpu,
        };
        u32 memTypeIndex = 0;
        VkCheck(vmaFindMemoryTypeIndexForBufferInfo(
            allocator->handle, &bufferInfo, &allocInfo, &memTypeIndex));
        const VmaPoolCreateInfo poolInfo =
        {
            .memoryTypeIndex = memTypeIndex,
            .frameInUseCount = kFramesInFlight - 1,
        };
        VmaPool pool = NULL;
        VkCheck(vmaCreatePool(allocator->handle, &poolInfo, &pool));
        ASSERT(pool);
        allocator->uavPool = pool;
        if (!pool)
        {
            success = false;
            goto cleanup;
        }
    }

    {
        const u32 queueFamilies[] =
        {
            g_vkr.queues[vkrQueueId_Gfx].family,
        };
        VkImageCreateInfo imageInfo = { 0 };
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        imageInfo.extent.width = 1024;
        imageInfo.extent.height = 1024;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 10;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage =
            VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.queueFamilyIndexCount = NELEM(queueFamilies);
        imageInfo.pQueueFamilyIndices = queueFamilies;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        const VmaAllocationCreateInfo allocInfo =
        {
            .flags = VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT,
            .usage = vkrMemUsage_GpuOnly,
        };
        u32 memTypeIndex = 0;
        VkCheck(vmaFindMemoryTypeIndexForImageInfo(
            allocator->handle, &imageInfo, &allocInfo, &memTypeIndex));
        const VmaPoolCreateInfo poolInfo =
        {
            .memoryTypeIndex = memTypeIndex,
            .frameInUseCount = kFramesInFlight - 1,
        };
        VmaPool pool = NULL;
        VkCheck(vmaCreatePool(allocator->handle, &poolInfo, &pool));
        ASSERT(pool);
        allocator->texturePool = pool;
        if (!pool)
        {
            success = false;
            goto cleanup;
        }
    }

cleanup:
    if (!success)
    {
        vkrMemSys_Shutdown();
    }

    return success;
}

void vkrMemSys_Shutdown(void)
{
    vkrAllocator *const allocator = &ms_inst;
    if (allocator->handle)
    {
        vkrMemSys_Finalize();
        if (allocator->stagePool)
        {
            vmaDestroyPool(allocator->handle, allocator->stagePool);
        }
        if (allocator->uavPool)
        {
            vmaDestroyPool(allocator->handle, allocator->uavPool);
        }
        if (allocator->cpuMeshPool)
        {
            vmaDestroyPool(allocator->handle, allocator->cpuMeshPool);
        }
        if (allocator->gpuMeshPool)
        {
            vmaDestroyPool(allocator->handle, allocator->gpuMeshPool);
        }
        if (allocator->texturePool)
        {
            vmaDestroyPool(allocator->handle, allocator->texturePool);
        }
        vmaDestroyAllocator(allocator->handle);
    }
    Spinlock_Del(&allocator->lock);
    memset(allocator, 0, sizeof(*allocator));
}

ProfileMark(pm_allocupdate, vkrMemSys_Update)
void vkrMemSys_Update(void)
{
    ProfileBegin(pm_allocupdate);

    vkrAllocator *const allocator = &ms_inst;
    if (allocator->handle)
    {
        const u32 frame = vkrSys_FrameIndex();
        vmaSetCurrentFrameIndex(allocator->handle, frame);
        {
            Spinlock_Lock(&allocator->lock);
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
            Spinlock_Unlock(&allocator->lock);

            FinalizeCheck(len);
        }
    }

    ProfileEnd(pm_allocupdate);
}

void vkrMemSys_Finalize(void)
{
    vkrDevice_WaitIdle();

    vkrAllocator *const allocator = &ms_inst;
    ASSERT(allocator->handle);

    Spinlock_Lock(&allocator->lock);
    const u32 frame = vkrSys_FrameIndex() + kFramesInFlight * 2;
    i32 len = allocator->numreleasable;
    vkrReleasable *const releasables = allocator->releasables;
    for (i32 i = len - 1; i >= 0; --i)
    {
        if (vkrReleasable_Del(&releasables[i], frame))
        {
            releasables[i] = releasables[--len];
        }
        else
        {
            ASSERT(false);
        }
    }
    allocator->numreleasable = len;
    Spinlock_Unlock(&allocator->lock);
}

// ----------------------------------------------------------------------------

ProfileMark(pm_bufnew, vkrMem_BufferNew)
bool vkrMem_BufferNew(
    vkrBuffer *const buffer,
    i32 size,
    VkBufferUsageFlags usage,
    vkrMemUsage memUsage)
{
    ProfileBegin(pm_bufnew);

    ASSERT(ms_inst.handle);
    ASSERT(size >= 0);
    ASSERT(buffer);
    memset(buffer, 0, sizeof(*buffer));

    const VkBufferCreateInfo bufferInfo =
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    const VmaAllocationCreateInfo allocInfo =
    {
        .flags = VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT,
        .usage = memUsage,
        .pool = GetBufferPool(usage, memUsage),
    };

    VkBuffer handle = NULL;
    VmaAllocation allocation = NULL;
    VkCheck(vmaCreateBuffer(
        ms_inst.handle,
        &bufferInfo,
        &allocInfo,
        &handle,
        &allocation,
        NULL));
    ASSERT(handle);

    if (handle)
    {
        buffer->handle = handle;
        buffer->allocation = allocation;
        buffer->size = size;
    }
    else
    {
        Con_Logf(LogSev_Error, "vkr", "Failed to allocate buffer:");
        Con_Logf(LogSev_Error, "vkr", "Size: %d", size);
    }
    ProfileEnd(pm_bufnew);

    return handle != NULL;
}

ProfileMark(pm_bufdel, vkrMem_BufferDel)
void vkrMem_BufferDel(vkrBuffer *const buffer)
{
    ProfileBegin(pm_bufdel);

    if (buffer->handle)
    {
        ASSERT(ms_inst.handle);
        vmaDestroyBuffer(
            ms_inst.handle,
            buffer->handle,
            buffer->allocation);
    }
    memset(buffer, 0, sizeof(*buffer));

    ProfileEnd(pm_bufdel);
}

// ----------------------------------------------------------------------------

ProfileMark(pm_imgnew, vkrMem_ImageNew)
bool vkrMem_ImageNew(
    vkrImage* image,
    const VkImageCreateInfo* info,
    vkrMemUsage memUsage)
{
    ProfileBegin(pm_imgnew);

    ASSERT(ms_inst.handle);
    memset(image, 0, sizeof(*image));

    const VmaAllocationCreateInfo allocInfo =
    {
        .flags = VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT,
        .usage = memUsage,
        .pool = GetTexturePool(info->usage, memUsage),
    };

    VkImage handle = NULL;
    VmaAllocation allocation = NULL;
    VkCheck(vmaCreateImage(
        ms_inst.handle,
        info,
        &allocInfo,
        &handle,
        &allocation,
        NULL));

    if (handle)
    {
        image->handle = handle;
        image->allocation = allocation;
        image->type = info->imageType;
        image->format = info->format;
        image->layout = info->initialLayout;
        image->usage = info->usage;
        image->width = info->extent.width;
        image->height = info->extent.height;
        image->depth = info->extent.depth;
        image->mipLevels = info->mipLevels;
        image->arrayLayers = info->arrayLayers;
    }
    else
    {
        Con_Logf(LogSev_Error, "vkr", "Failed to allocate image:");
        Con_Logf(LogSev_Error, "vkr", "Size: %d x %d x %d", info->extent.width, info->extent.height, info->extent.depth);
        Con_Logf(LogSev_Error, "vkr", "Mip Levels: %d", info->mipLevels);
        Con_Logf(LogSev_Error, "vkr", "Array Layers: %d", info->arrayLayers);
    }

    ProfileEnd(pm_imgnew);
    return handle != NULL;
}

ProfileMark(pm_imgdel, vkrMem_ImageDel)
void vkrMem_ImageDel(vkrImage* image)
{
    ProfileBegin(pm_imgdel);

    if (image->handle)
    {
        ASSERT(ms_inst.handle);
        vmaDestroyImage(
            ms_inst.handle,
            image->handle,
            image->allocation);
    }
    memset(image, 0, sizeof(*image));

    ProfileEnd(pm_imgdel);
}

// ----------------------------------------------------------------------------

ProfileMark(pm_releasableadd, vkrReleasable_Add)
void vkrReleasable_Add(
    vkrReleasable const *const releasable)
{
    ProfileBegin(pm_releasableadd);

    vkrAllocator *const allocator = &ms_inst;
    ASSERT(allocator->handle);
    ASSERT(releasable);

    Spinlock_Lock(&allocator->lock);
    i32 back = allocator->numreleasable++;
    Perm_Reserve(allocator->releasables, back + 1);
    allocator->releasables[back] = *releasable;
    Spinlock_Unlock(&allocator->lock);

    FinalizeCheck(back + 1);

    ProfileEnd(pm_releasableadd);
}

ProfileMark(pm_releasabledel, vkrReleasable_Del)
bool vkrReleasable_Del(
    vkrReleasable *const releasable,
    u32 frame)
{
    ProfileBegin(pm_releasabledel);

    ASSERT(releasable);
    u32 duration = frame - releasable->frame;
    bool ready = duration > kFramesInFlight;
    if (ready)
    {
        switch (releasable->type)
        {
        default:
            ASSERT(false);
            break;
        case vkrReleasableType_Buffer:
            vkrMem_BufferDel(&releasable->buffer);
            break;
        case vkrReleasableType_Image:
            vkrMem_ImageDel(&releasable->image);
            break;
        case vkrReleasableType_ImageView:
            if (releasable->view)
            {
                vkDestroyImageView(g_vkr.dev, releasable->view, NULL);
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
    VkFence fence = NULL;
    VkQueue queue = NULL;
    VkCommandBuffer cmd = vkrContext_GetTmpCmd(id, &fence, &queue);
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

ProfileMark(pm_memmap, vkrMem_Map)
void *const vkrMem_Map(VmaAllocation allocation)
{
    ProfileBegin(pm_memmap);
    ASSERT(allocation);

    void* result = NULL;
    VkCheck(vmaMapMemory(ms_inst.handle, allocation, &result));
    ASSERT(result);

    ProfileEnd(pm_memmap);
    return result;
}

ProfileMark(pm_memunmap, vkrMem_Unmap)
void vkrMem_Unmap(VmaAllocation allocation)
{
    ProfileBegin(pm_memunmap);
    ASSERT(allocation);

    vmaUnmapMemory(ms_inst.handle, allocation);

    ProfileEnd(pm_memunmap);
}

ProfileMark(pm_memflush, vkrMem_Flush)
void vkrMem_Flush(VmaAllocation allocation)
{
    ProfileBegin(pm_memflush);
    ASSERT(allocation);

    const VkDeviceSize offset = 0;
    const VkDeviceSize size = VK_WHOLE_SIZE;
    VkCheck(vmaFlushAllocation(ms_inst.handle, allocation, offset, size));
    VkCheck(vmaInvalidateAllocation(ms_inst.handle, allocation, offset, size));

    ProfileEnd(pm_memflush);
}

static VmaPool GetBufferPool(VkBufferUsageFlags usage, vkrMemUsage memUsage)
{
    VmaPool pool = NULL;
    const VkBufferUsageFlags kMeshUsage =
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    const VkBufferUsageFlags kShaderUsage =
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    switch (memUsage)
    {
    default:
        break;
    case vkrMemUsage_CpuOnly:
        pool = ms_inst.stagePool;
        break;
    case vkrMemUsage_CpuToGpu:
        if (usage & kMeshUsage)
        {
            pool = ms_inst.cpuMeshPool;
        }
        else if (usage & kShaderUsage)
        {
            pool = ms_inst.uavPool;
        }
        break;
    case vkrMemUsage_GpuOnly:
        if (usage & kMeshUsage)
        {
            pool = ms_inst.gpuMeshPool;
        }
        break;
    }
    ASSERT(pool);
    return pool;
}

static VmaPool GetTexturePool(VkImageUsageFlags usage, vkrMemUsage memUsage)
{
    return ms_inst.texturePool;
}

static void FinalizeCheck(i32 len)
{
    if (len >= ConVar_GetInt(&cv_r_maxdelqueue))
    {
        Con_Logf(LogSev_Warning, "vkr", "Too many gpu objects, force-finalizing");
        vkrMemSys_Finalize();
    }
}
