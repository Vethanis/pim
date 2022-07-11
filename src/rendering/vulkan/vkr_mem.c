#include "rendering/vulkan/vkr_mem.h"

#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_sync.h"
#include "rendering/vulkan/vkr_device.h"
#include "rendering/vulkan/vkr_context.h"
#include "rendering/vulkan/vkr_image.h"
#include "rendering/vulkan/vkr_framebuffer.h"

#include "allocator/allocator.h"
#include "common/profiler.h"
#include "common/time.h"
#include "common/console.h"
#include "common/cvars.h"
#include "threading/task.h"
#include "threading/mutex.h"
#include "math/scalar.h"

#include "VulkanMemoryAllocator/src/vk_mem_alloc.h"
#include <string.h>

typedef struct vkrMemPool_s
{
    VmaPool handle;
    VkDeviceSize size;
    VkBufferUsageFlags bufferUsage;
    VkImageUsageFlags imageUsage;
    vkrMemUsage memUsage;
    vkrQueueFlags queueUsage;
} vkrMemPool;

typedef struct vkrAllocator_s
{
    Mutex lock;
    VmaAllocator handle;
    vkrMemPool stagingPool;
    vkrMemPool deviceBufferPool;
    vkrMemPool dynamicBufferPool;
    vkrMemPool deviceTexturePool;
    vkrReleasable* releasables;
    i32 numreleasable;
} vkrAllocator;

static vkrAllocator ms_inst;

static bool vkrMemPool_New(
    vkrMemPool* pool,
    VkDeviceSize size,
    VkBufferUsageFlags bufferUsage,
    VkImageUsageFlags imageUsage,
    vkrMemUsage memUsage,
    vkrQueueFlags queueUsage);
static void vkrMemPool_Del(vkrMemPool* pool);
static VmaPool GetBufferPool(VkBufferUsageFlags usage, vkrMemUsage memUsage);
static VmaPool GetTexturePool(VkImageUsageFlags usage, vkrMemUsage memUsage);
static void FinalizeCheck(i32 len);

bool vkrMemSys_Init(void)
{
    bool success = true;

    vkrAllocator *const allocator = &ms_inst;
    memset(allocator, 0, sizeof(*allocator));
    Mutex_New(&allocator->lock);

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
            .frameInUseCount = R_ResourceSets - 1,
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

    if (!vkrMemPool_New(
        &ms_inst.stagingPool,
        1 << 20,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        0x0,
        vkrMemUsage_CpuOnly,
        vkrQueueFlag_GraphicsBit))
    {
        success = false;
        goto cleanup;
    }

    if (!vkrMemPool_New(
        &ms_inst.deviceBufferPool,
        1 << 20,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        0x0,
        vkrMemUsage_GpuOnly,
        vkrQueueFlag_GraphicsBit))
    {
        success = false;
        goto cleanup;
    }

    if (!vkrMemPool_New(
        &ms_inst.dynamicBufferPool,
        1 << 20,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        0x0,
        vkrMemUsage_Dynamic,
        vkrQueueFlag_GraphicsBit))
    {
        success = false;
        goto cleanup;
    }

    if (!vkrMemPool_New(
        &ms_inst.deviceTexturePool,
        1024,
        0x0,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT |
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
        VK_IMAGE_USAGE_SAMPLED_BIT |
        VK_IMAGE_USAGE_STORAGE_BIT,
        vkrMemUsage_GpuOnly,
        vkrQueueFlag_GraphicsBit))
    {
        success = false;
        goto cleanup;
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
        vkrMemPool_Del(&ms_inst.stagingPool);
        vkrMemPool_Del(&ms_inst.deviceTexturePool);
        vkrMemPool_Del(&ms_inst.deviceBufferPool);
        vkrMemPool_Del(&ms_inst.dynamicBufferPool);
        vmaDestroyAllocator(allocator->handle);
    }
    Mutex_Del(&allocator->lock);
    memset(allocator, 0, sizeof(*allocator));
}

ProfileMark(pm_allocupdate, vkrMemSys_Update)
void vkrMemSys_Update(void)
{
    ProfileBegin(pm_allocupdate);

    vkrAllocator *const allocator = &ms_inst;
    if (allocator->handle)
    {
        const u32 frame = vkrGetFrameCount();
        vmaSetCurrentFrameIndex(allocator->handle, frame);
        {
            Mutex_Lock(&allocator->lock);
            i32 len = allocator->numreleasable;
            vkrReleasable* releasables = allocator->releasables;
            for (i32 i = len - 1; i >= 0; --i)
            {
                if (vkrSubmit_Poll(releasables[i].submitId))
                {
                    vkrReleasable_Del(&releasables[i]);
                    releasables[i] = releasables[len - 1];
                    --len;
                }
            }
            allocator->numreleasable = len;
            Mutex_Unlock(&allocator->lock);

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

    Mutex_Lock(&allocator->lock);
    i32 len = allocator->numreleasable;
    vkrReleasable *const releasables = allocator->releasables;
    for (i32 i = len - 1; i >= 0; --i)
    {
        vkrSubmit_Await(releasables[i].submitId);
        vkrReleasable_Del(&releasables[i]);
        releasables[i] = releasables[--len];
    }
    allocator->numreleasable = len;
    Mutex_Unlock(&allocator->lock);
}

// ----------------------------------------------------------------------------

ProfileMark(pm_bufnew, vkrMem_BufferNew)
bool vkrMem_BufferNew(
    vkrBuffer* buffer,
    i32 size,
    VkBufferUsageFlags usage,
    vkrMemUsage memUsage)
{
    ProfileBegin(pm_bufnew);

    ASSERT(ms_inst.handle);
    ASSERT(size >= 0);
    ASSERT(buffer);
    memset(buffer, 0, sizeof(*buffer));
    buffer->state.owner = vkrQueueId_Graphics;

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
        .usage = (VmaMemoryUsage)memUsage,
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
void vkrMem_BufferDel(vkrBuffer* buffer)
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

    ASSERT(g_vkr.dev);
    ASSERT(ms_inst.handle);
    memset(image, 0, sizeof(*image));
    bool success = true;

    image->type = info->imageType;
    image->format = info->format;
    image->state.owner = vkrQueueId_Graphics;
    image->state.stage = 0;
    image->state.layout = info->initialLayout;
    image->usage = info->usage;
    image->width = info->extent.width;
    image->height = info->extent.height;
    image->depth = info->extent.depth;
    image->mipLevels = info->mipLevels;
    image->arrayLayers = info->arrayLayers;
    image->imported = 0;

    const VmaAllocationCreateInfo allocInfo =
    {
        .flags = VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT,
        .usage = (VmaMemoryUsage)memUsage,
        .pool = GetTexturePool(info->usage, memUsage),
    };

    VkCheck(vmaCreateImage(
        ms_inst.handle,
        info,
        &allocInfo,
        &image->handle,
        &image->allocation,
        NULL));
    if (!image->handle)
    {
        success = false;
        goto cleanup;
    }

    VkImageViewCreateInfo viewInfo;
    if (vkrImage_InfoToViewInfo(info, &viewInfo))
    {
        viewInfo.image = image->handle;
        VkCheck(vkCreateImageView(g_vkr.dev, &viewInfo, NULL, &image->view));
        if (!image->view)
        {
            success = false;
            goto cleanup;
        }
    }

cleanup:
    if (!success)
    {
        Con_Logf(LogSev_Error, "vkr", "Failed to allocate image:");
        Con_Logf(LogSev_Error, "vkr", "Size: %d x %d x %d", info->extent.width, info->extent.height, info->extent.depth);
        Con_Logf(LogSev_Error, "vkr", "Mip Levels: %d", info->mipLevels);
        Con_Logf(LogSev_Error, "vkr", "Array Layers: %d", info->arrayLayers);
        vkrMem_ImageDel(image);
    }

    ProfileEnd(pm_imgnew);
    return success;
}

ProfileMark(pm_imgdel, vkrMem_ImageDel)
void vkrMem_ImageDel(vkrImage* image)
{
    ProfileBegin(pm_imgdel);

    if (image->view)
    {
        ASSERT(g_vkr.dev);
        vkDestroyImageView(g_vkr.dev, image->view, NULL);
    }
    if (image->allocation)
    {
        ASSERT(!image->imported);
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
void vkrReleasable_Add(const vkrReleasable* releasable)
{
    ProfileBegin(pm_releasableadd);

    vkrAllocator *const allocator = &ms_inst;
    ASSERT(allocator->handle);
    ASSERT(releasable);
    ASSERT(releasable->submitId.valid);

    Mutex_Lock(&allocator->lock);
    i32 back = allocator->numreleasable++;
    Perm_Reserve(allocator->releasables, back + 1);
    allocator->releasables[back] = *releasable;
    Mutex_Unlock(&allocator->lock);

    FinalizeCheck(back + 1);

    ProfileEnd(pm_releasableadd);
}

ProfileMark(pm_releasabledel, vkrReleasable_Del)
void vkrReleasable_Del(vkrReleasable* releasable)
{
    ProfileBegin(pm_releasabledel);

    ASSERT(releasable);
    ASSERT(ms_inst.handle);
    ASSERT(g_vkr.dev);

    switch (releasable->type)
    {
    default:
        ASSERT(false);
        break;
    case vkrReleasableType_Buffer:
        if (releasable->buffer.handle)
        {
            vmaDestroyBuffer(
                ms_inst.handle,
                releasable->buffer.handle,
                releasable->buffer.allocation);
        }
        break;
    case vkrReleasableType_Image:
        if (releasable->image.view)
        {
            vkDestroyImageView(g_vkr.dev, releasable->image.view, NULL);
        }
        if (releasable->image.allocation)
        {
            vmaDestroyImage(
                ms_inst.handle,
                releasable->image.handle,
                releasable->image.allocation);
        }
        break;
    case vkrReleasableType_ImageView:
        if (releasable->view)
        {
            vkDestroyImageView(g_vkr.dev, releasable->view, NULL);
        }
        break;
    case vkrReleasableType_Attachment:
        if (releasable->view)
        {
            vkrFramebuffer_Remove(releasable->view);
            vkDestroyImageView(g_vkr.dev, releasable->view, NULL);
        }
        break;
    }
    memset(releasable, 0, sizeof(*releasable));

    ProfileEnd(pm_releasabledel);
}

ProfileMark(pm_memmap, vkrMem_Map)
void* vkrMem_Map(VmaAllocation allocation)
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

static bool vkrMemPool_New(
    vkrMemPool* pool,
    VkDeviceSize size,
    VkBufferUsageFlags bufferUsage,
    VkImageUsageFlags imageUsage,
    vkrMemUsage memUsage,
    vkrQueueFlags queueUsage)
{
    memset(pool, 0, sizeof(*pool));

    ASSERT(size);
    ASSERT((bufferUsage && !imageUsage) || (imageUsage && !bufferUsage));
    ASSERT(memUsage);
    ASSERT(queueUsage);

    u32 queueFamilyCount = 0;
    u32 queueFamilies[vkrQueueId_COUNT] = { 0 };
    if (queueUsage & vkrQueueFlag_GraphicsBit)
    {
        queueFamilies[queueFamilyCount++] = g_vkr.queues[vkrQueueId_Graphics].family;
    }

    if (bufferUsage)
    {
        const VkBufferCreateInfo bufferInfo =
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = size,
            .usage = bufferUsage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = queueFamilyCount,
            .pQueueFamilyIndices = queueFamilies,
        };
        const VmaAllocationCreateInfo allocInfo =
        {
            .flags = VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT,
            .usage = (VmaMemoryUsage)memUsage,
        };
        u32 memTypeIndex = 0;
        VkCheck(vmaFindMemoryTypeIndexForBufferInfo(
            ms_inst.handle, &bufferInfo, &allocInfo, &memTypeIndex));
        const VmaPoolCreateInfo poolInfo =
        {
            .memoryTypeIndex = memTypeIndex,
            .frameInUseCount = R_ResourceSets - 1,
        };
        VkCheck(vmaCreatePool(ms_inst.handle, &poolInfo, &pool->handle));
        ASSERT(pool->handle);
        if (pool->handle)
        {
            pool->size = size;
            pool->bufferUsage = bufferUsage;
            pool->memUsage = memUsage;
            pool->queueUsage = queueUsage;
        }
        return pool->handle != NULL;
    }
    else if (imageUsage)
    {
        VkImageCreateInfo imageInfo = { 0 };
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
        imageInfo.extent.width = (u32)size;
        imageInfo.extent.height = (u32)size;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1 + u64_log2(size);
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = imageUsage;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.queueFamilyIndexCount = queueFamilyCount;
        imageInfo.pQueueFamilyIndices = queueFamilies;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        const VmaAllocationCreateInfo allocInfo =
        {
            .flags = VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT,
            .usage = (VmaMemoryUsage)memUsage,
        };
        u32 memTypeIndex = 0;
        VkCheck(vmaFindMemoryTypeIndexForImageInfo(
            ms_inst.handle, &imageInfo, &allocInfo, &memTypeIndex));
        const VmaPoolCreateInfo poolInfo =
        {
            .memoryTypeIndex = memTypeIndex,
            .frameInUseCount = R_ResourceSets,
        };
        VkCheck(vmaCreatePool(ms_inst.handle, &poolInfo, &pool->handle));
        ASSERT(pool->handle);
        if (pool->handle)
        {
            pool->size = size;
            pool->imageUsage = imageUsage;
            pool->memUsage = memUsage;
            pool->queueUsage = queueUsage;
        }
        return pool->handle != NULL;
    }
    else
    {
        return false;
    }
}

static void vkrMemPool_Del(vkrMemPool* pool)
{
    if (pool->handle)
    {
        vmaDestroyPool(ms_inst.handle, pool->handle);
    }
    memset(pool, 0, sizeof(*pool));
}

static VmaPool GetBufferPool(VkBufferUsageFlags usage, vkrMemUsage memUsage)
{
    VmaPool pool = NULL;
    switch (memUsage)
    {
    default:
        break;
    case vkrMemUsage_CpuOnly:
        ASSERT(usage & ms_inst.stagingPool.bufferUsage);
        pool = ms_inst.stagingPool.handle;
        break;
    case vkrMemUsage_Dynamic:
        ASSERT(usage & ms_inst.dynamicBufferPool.bufferUsage);
        pool = ms_inst.dynamicBufferPool.handle;
        break;
    case vkrMemUsage_GpuOnly:
        ASSERT(usage & ms_inst.deviceBufferPool.bufferUsage);
        pool = ms_inst.deviceBufferPool.handle;
        break;
    }
    ASSERT(pool);
    return pool;
}

static VmaPool GetTexturePool(VkImageUsageFlags usage, vkrMemUsage memUsage)
{
    VmaPool pool = NULL;
    switch (memUsage)
    {
    default:
        break;
    case vkrMemUsage_GpuOnly:
        if (usage & ms_inst.deviceTexturePool.imageUsage)
        {
            pool = ms_inst.deviceTexturePool.handle;
        }
        break;
    }
    const VkImageUsageFlags attachmentUsage =
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    ASSERT(pool || (usage & attachmentUsage));
    return pool;
}

static void FinalizeCheck(i32 len)
{
    if (len >= ConVar_GetInt(&cv_r_maxdelqueue))
    {
        Con_Logf(LogSev_Warning, "vkr", "Too many gpu objects, force-finalizing");
        vkrContext* ctx = vkrGetContext();
        for (i32 id = 0; id < NELEM(ctx->curCmdBuf); ++id)
        {
            vkrCmdBuf* cmd = &ctx->curCmdBuf[id];
            if (cmd->began)
            {
                vkrCmdSubmit(cmd, NULL, 0, NULL);
            }
        }
        vkrSubmit_AwaitAll();
        vkrMemSys_Update();
    }
}
