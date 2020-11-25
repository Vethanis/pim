#include "rendering/framegraph/framegraph.hpp"
#include "rendering/vulkan/vkr_buffer.h"
#include "rendering/vulkan/vkr_image.h"
#include "rendering/vulkan/vkr_context.h"
#include "rendering/vulkan/vkr_cmd.h"

namespace vkr
{
    static constexpr VkAccessFlags kWriteAccess =
        VK_ACCESS_SHADER_WRITE_BIT |
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
        VK_ACCESS_TRANSFER_WRITE_BIT |
        VK_ACCESS_HOST_WRITE_BIT |
        VK_ACCESS_MEMORY_WRITE_BIT |
        VK_ACCESS_TRANSFORM_FEEDBACK_WRITE_BIT_EXT |
        VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT |
        VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR |
        VK_ACCESS_COMMAND_PREPROCESS_WRITE_BIT_NV;

    static constexpr VkAccessFlags kReadAccess =
        VK_ACCESS_INDIRECT_COMMAND_READ_BIT |
        VK_ACCESS_INDEX_READ_BIT |
        VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
        VK_ACCESS_UNIFORM_READ_BIT |
        VK_ACCESS_INPUT_ATTACHMENT_READ_BIT |
        VK_ACCESS_SHADER_READ_BIT |
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
        VK_ACCESS_TRANSFER_READ_BIT |
        VK_ACCESS_HOST_READ_BIT |
        VK_ACCESS_MEMORY_READ_BIT |
        VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT |
        VK_ACCESS_CONDITIONAL_RENDERING_READ_BIT_EXT |
        VK_ACCESS_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT |
        VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR |
        VK_ACCESS_SHADING_RATE_IMAGE_READ_BIT_NV |
        VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT |
        VK_ACCESS_COMMAND_PREPROCESS_READ_BIT_NV;

    // ------------------------------------------------------------------------
    // BufferInfo

    void BufferInfo::Merge(const BufferInfo& other)
    {
        usage |= other.usage;
        ASSERT(size == other.size);
        ASSERT(memUsage == other.memUsage);
    }

    // ------------------------------------------------------------------------
    // ImageInfo

    void ImageInfo::Merge(const ImageInfo& other)
    {
        info.usage |= other.info.usage;
        ASSERT(memUsage == other.memUsage);
        ASSERT(info.extent.width == other.info.extent.width);
        ASSERT(info.extent.height == other.info.extent.height);
        ASSERT(info.extent.depth == other.info.extent.depth);
    }

    // ------------------------------------------------------------------------
    // Buffer

    bool Buffer::Create(const BufferInfo& info)
    {
        ASSERT(!frames[0].handle);
        ASSERT(info.size > 0);
        for (i32 i = 0; i < kFramesInFlight; ++i)
        {
            if (!vkrBuffer_New(&frames[i], info.size, info.usage, info.memUsage, PIM_FILELINE))
            {
                Release();
                return false;
            }
        }
        return true;
    }

    void Buffer::Release()
    {
        for (i32 i = 0; i < kFramesInFlight; ++i)
        {
            vkrBuffer_Release(&frames[i], NULL);
        }
    }

    // ------------------------------------------------------------------------
    // Image

    bool Image::Create(const ImageInfo& info)
    {
        ASSERT(!frames[0].handle);
        for (i32 i = 0; i < kFramesInFlight; ++i)
        {
            if (!vkrImage_New(&frames[i], &info.info, info.memUsage, PIM_FILELINE))
            {
                Release();
                return false;
            }
        }
        return true;
    }

    void Image::Release()
    {
        for (i32 i = 0; i < kFramesInFlight; ++i)
        {
            vkrImage_Release(&frames[i], NULL);
        }
    }

    // ------------------------------------------------------------------------
    // BufferTable

    BufferTable::~BufferTable()
    {
        for (Buffer& buffer : m_buffers)
        {
            buffer.Release();
        }
    }

    i32 BufferTable::Alloc(const BufferInfo& info, Hash64 name)
    {
        i32 slot = m_names.Find(name);
        if (slot >= 0)
        {
            if (m_infos[slot] != info)
            {
                Free(slot);
                slot = -1;
            }
        }
        if (slot < 0)
        {
            slot = m_names.Size();
            m_names.Grow() = name;
            m_infos.Grow() = info;
            m_buffers.Grow().Create(info);
            m_frames.Grow();
            m_states.Grow();
        }
        m_frames[slot] = vkr_frameIndex();
        return slot;
    }

    void BufferTable::Free(i32 slot)
    {
        m_buffers[slot].Release();
        m_names.Remove(slot);
        m_infos.Remove(slot);
        m_buffers.Remove(slot);
        m_frames.Remove(slot);
        m_states.Remove(slot);
    }

    void BufferTable::BeforeSetup()
    {
        m_logicalNames.Clear();
        m_logicalInfos.Clear();

        const u32 curFrame = vkr_frameIndex();
        for (i32 i = m_buffers.Size() - 1; i >= 0; --i)
        {
            u32 duration = (curFrame - m_frames[i]);
            if ((duration > kFramesInFlight))
            {
                Free(i);
            }
        }
    }

    ResourceId BufferTable::GetLogical(const BufferInfo& info, const char* name)
    {
        const Hash64 hash(name);
        ASSERT(!hash.IsNull());

        i32 slot = m_logicalNames.Find(hash);
        if (slot < 0)
        {
            slot = m_logicalNames.Size();
            m_logicalNames.Grow() = hash;
            m_logicalInfos.Grow() = info;
        }
        m_logicalInfos[slot].Merge(info);

        ResourceId rid = {};
        rid.type = ResourceType_Buffer;
        rid.syncIndex = vkr_syncIndex();
        rid.name = hash;

        return rid;
    }

    void BufferTable::BeforeExecute()
    {
        const i32 logicalLen = m_logicalNames.Size();
        for (i32 i = 0; i < logicalLen; ++i)
        {
            Alloc(m_logicalInfos[i], m_logicalNames[i]);
        }
    }

    vkrBuffer& BufferTable::GetPhysical(ResourceId rid)
    {
        ASSERT(rid.type == ResourceType_Buffer);
        i32 slot = m_names.Find(rid.name);
        m_frames[slot] = vkr_frameIndex();
        return m_buffers[slot].frames[rid.syncIndex];
    }

    ResourceState& BufferTable::GetState(ResourceId rid)
    {
        i32 slot = m_names.Find(rid.name);
        return m_states[slot].frames[rid.syncIndex];
    }

    // ------------------------------------------------------------------------
    // ImageTable

    ImageTable::~ImageTable()
    {
        for (Image& image : m_images)
        {
            image.Release();
        }
    }

    i32 ImageTable::Alloc(const ImageInfo& info, Hash64 name)
    {
        i32 slot = m_names.Find(name);
        if (slot >= 0)
        {
            if (m_infos[slot] != info)
            {
                Free(slot);
                slot = -1;
            }
        }
        if (slot < 0)
        {
            slot = m_names.Size();
            m_names.Grow() = name;
            m_infos.Grow() = info;
            m_images.Grow().Create(info);
            m_frames.Grow();
            m_states.Grow();
        }
        m_frames[slot] = vkr_frameIndex();
        return slot;
    }

    void ImageTable::Free(i32 slot)
    {
        m_images[slot].Release();
        m_names.Remove(slot);
        m_infos.Remove(slot);
        m_images.Remove(slot);
        m_frames.Remove(slot);
        m_states.Remove(slot);
    }

    void ImageTable::BeforeSetup()
    {
        m_logicalNames.Clear();
        m_logicalInfos.Clear();

        const u32 curFrame = vkr_frameIndex();
        for (i32 i = m_images.Size() - 1; i >= 0; --i)
        {
            u32 duration = (curFrame - m_frames[i]);
            if ((duration > kFramesInFlight))
            {
                Free(i);
            }
        }
    }

    ResourceId ImageTable::GetLogical(const ImageInfo& info, const char* name)
    {
        const Hash64 hash(name);
        ASSERT(!hash.IsNull());

        i32 slot = m_logicalNames.Find(hash);
        if (slot < 0)
        {
            slot = m_logicalNames.Size();
            m_logicalNames.Grow() = hash;
            m_logicalInfos.Grow() = info;
        }
        m_logicalInfos[slot].Merge(info);

        ResourceId rid = {};
        rid.type = ResourceType_Buffer;
        rid.syncIndex = vkr_syncIndex();
        rid.name = hash;

        return rid;
    }

    void ImageTable::BeforeExecute()
    {
        const i32 logicalLen = m_logicalNames.Size();
        for (i32 i = 0; i < logicalLen; ++i)
        {
            Alloc(m_logicalInfos[i], m_logicalNames[i]);
        }
    }

    vkrImage& ImageTable::GetPhysical(ResourceId rid)
    {
        ASSERT(rid.type == ResourceType_Image);
        i32 slot = m_names.Find(rid.name);
        m_frames[slot] = vkr_frameIndex();
        return m_images[slot].frames[rid.syncIndex];
    }

    ResourceState& ImageTable::GetState(ResourceId rid)
    {
        i32 slot = m_names.Find(rid.name);
        return m_states[slot].frames[rid.syncIndex];
    }

    // ------------------------------------------------------------------------
    // RenderPass

    ResourceId RenderPass::GetBuffer(
        const BufferInfo& info,
        const char* name,
        VkPipelineStageFlags stageFlags,
        VkAccessFlags accessMask)
    {
        ResourceId rid = m_graph->GetBuffer(info, name);
        ResourceAccess& ra = m_accesses.Grow();
        ra.resource = rid;
        ra.state.queue = m_queue;
        ra.state.access = accessMask;
        ra.state.stage = stageFlags;
        ra.state.layout = VK_IMAGE_LAYOUT_UNDEFINED;
        return rid;
    }

    ResourceId RenderPass::GetImage(
        const ImageInfo& info,
        const char* name,
        VkPipelineStageFlags stageFlags,
        VkAccessFlags accessMask,
        VkImageLayout layout)
    {
        ResourceId rid = m_graph->GetImage(info, name);
        ResourceAccess& ra = m_accesses.Grow();
        ra.resource = rid;
        ra.state.queue = m_queue;
        ra.state.access = accessMask;
        ra.state.stage = stageFlags;
        ra.state.layout = layout;
        return rid;
    }

    vkrBuffer& RenderPass::ResolveBuffer(ResourceId id)
    {
        ASSERT(m_graph);
        return m_graph->ResolveBuffer(id);
    }

    vkrImage& RenderPass::ResolveImage(ResourceId id)
    {
        ASSERT(m_graph);
        return m_graph->ResolveImage(id);
    }

    ResourceId RenderPass::ReadVertexBuffer(
        const BufferInfo& info,
        const char* name)
    {
        ASSERT(info.usage & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        return GetBuffer(
            info,
            name,
            VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
            VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT);
    }

    ResourceId RenderPass::ReadIndexBuffer(
        const BufferInfo& info,
        const char* name)
    {
        ASSERT(info.usage & VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
        return GetBuffer(
            info,
            name,
            VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
            VK_ACCESS_INDEX_READ_BIT);
    }

    ResourceId RenderPass::ReadIndirectBuffer(
        const BufferInfo& info,
        const char* name)
    {
        ASSERT(info.usage & VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);
        return GetBuffer(
            info,
            name,
            VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
            VK_ACCESS_INDIRECT_COMMAND_READ_BIT);
    }

    ResourceId RenderPass::ReadUniformBuffer(
        const BufferInfo& info,
        const char* name,
        VkPipelineStageFlags stages)
    {
        ASSERT(info.usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
        return GetBuffer(
            info,
            name,
            stages,
            VK_ACCESS_UNIFORM_READ_BIT);
    }

    ResourceId RenderPass::ReadStorageBuffer(
        const BufferInfo& info,
        const char* name,
        VkPipelineStageFlags stages)
    {
        ASSERT(info.usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        return GetBuffer(
            info,
            name,
            stages,
            VK_ACCESS_SHADER_READ_BIT);
    }
    ResourceId RenderPass::WriteStorageBuffer(
        const BufferInfo& info,
        const char* name,
        VkPipelineStageFlags stages)
    {
        ASSERT(info.usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        return GetBuffer(
            info,
            name,
            stages,
            VK_ACCESS_SHADER_WRITE_BIT);
    }
    ResourceId RenderPass::ReadWriteStorageBuffer(
        const BufferInfo& info,
        const char* name,
        VkPipelineStageFlags stages)
    {
        ASSERT(info.usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        return GetBuffer(
            info,
            name,
            stages,
            VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);
    }

    ResourceId RenderPass::InputColorAttachment(
        const ImageInfo& info,
        const char* name)
    {
        ASSERT(info.info.usage & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
        ASSERT(info.info.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
        return GetImage(
            info,
            name,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_ACCESS_INPUT_ATTACHMENT_READ_BIT |
            VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    }
    ResourceId RenderPass::OutputColorAttachment(
        const ImageInfo& info,
        const char* name)
    {
        ASSERT(info.info.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
        return GetImage(
            info,
            name,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    }

    ResourceId RenderPass::InputDepthAttachment(
        const ImageInfo& info,
        const char* name)
    {
        ASSERT(info.info.usage & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
        ASSERT(info.info.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
        return GetImage(
            info,
            name,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_ACCESS_INPUT_ATTACHMENT_READ_BIT |
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
    }
    ResourceId RenderPass::OutputDepthAttachment(
        const ImageInfo& info,
        const char* name)
    {
        ASSERT(info.info.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
        return GetImage(
            info,
            name,
            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    }

    ResourceId RenderPass::SampleImage(
        const ImageInfo& info,
        const char* name,
        VkPipelineStageFlags stages)
    {
        ASSERT(info.info.usage & VK_IMAGE_USAGE_SAMPLED_BIT);
        return GetImage(
            info,
            name,
            stages,
            VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    ResourceId RenderPass::ReadStorageImage(
        const ImageInfo& info,
        const char* name,
        VkPipelineStageFlags stages)
    {
        ASSERT(info.info.usage & VK_IMAGE_USAGE_STORAGE_BIT);
        return GetImage(
            info,
            name,
            stages,
            VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_GENERAL);
    }
    ResourceId RenderPass::WriteStorageImage(
        const ImageInfo& info,
        const char* name,
        VkPipelineStageFlags stages)
    {
        ASSERT(info.info.usage & VK_IMAGE_USAGE_STORAGE_BIT);
        return GetImage(
            info,
            name,
            stages,
            VK_ACCESS_SHADER_WRITE_BIT,
            VK_IMAGE_LAYOUT_GENERAL);
    }
    ResourceId RenderPass::ReadWriteStorageImage(
        const ImageInfo& info,
        const char* name,
        VkPipelineStageFlags stages)
    {
        ASSERT(info.info.usage & VK_IMAGE_USAGE_STORAGE_BIT);
        return GetImage(
            info,
            name,
            stages,
            VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
            VK_IMAGE_LAYOUT_GENERAL);
    }

    ResourceId RenderPass::PresentImage(
        const ImageInfo& info,
        const char* name)
    {
        return GetImage(
            info,
            name,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            VK_ACCESS_MEMORY_READ_BIT,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    }

    // ------------------------------------------------------------------------
    // FrameGraph

    void FrameGraph::Begin()
    {
        m_buffers.BeforeSetup();
        m_images.BeforeSetup();
        m_passes.Clear();
    }

    void FrameGraph::AddPass(RenderPass* pass)
    {
        pass->m_accesses.Clear();
        pass->m_graph = this;
        if (pass->Setup())
        {
            m_passes.Grow() = pass;
        }
    }
    
    ResourceId FrameGraph::GetBuffer(const BufferInfo& info, const char* name)
    {
        return m_buffers.GetLogical(info, name);
    }

    ResourceId FrameGraph::GetImage(const ImageInfo& info, const char* name)
    {
        return m_images.GetLogical(info, name);
    }

    void FrameGraph::End()
    {
        m_buffers.BeforeExecute();
        m_images.BeforeExecute();

        VkCommandBuffer cmds[vkrQueueId_COUNT] = {};
        VkFence fences[vkrQueueId_COUNT] = {};
        VkQueue queues[vkrQueueId_COUNT] = {};
        bool hadTransfer[vkrQueueId_COUNT] = {};

        vkrThreadContext *const ctx = vkrContext_Get();
        for (i32 i = 0; i < vkrQueueId_COUNT; ++i)
        {
            cmds[i] = vkrContext_GetTmpCmd(ctx, (vkrQueueId)i, &fences[i], &queues[i]);
            vkrCmdBegin(cmds[i]);
            hadTransfer[i] = false;
        }

        for (RenderPass* pass : m_passes)
        {
            for (const ResourceAccess& access : pass->m_accesses)
            {
                const ResourceState& newState = access.state;
                switch (access.resource.type)
                {
                default:
                    ASSERT(false);
                    break;
                case ResourceType_Buffer:
                {
                    ResourceState& oldState = m_buffers.GetState(access.resource);
                    bool needTransfer = oldState.queue != newState.queue;
                    bool pendingWrites = (oldState.access & kWriteAccess) != 0x0;
                    bool pendingReads = (oldState.access & kReadAccess) != 0x0;
                    bool upcomingWrites = (newState.access & kWriteAccess) != 0x0;
                    bool upcomingReads = (newState.access & kReadAccess) != 0x0;
                    bool hazard =
                        (needTransfer) ||
                        (pendingWrites && (upcomingReads || upcomingWrites)) ||
                        (pendingReads && (upcomingWrites));
                    if (!hazard)
                    {
                        oldState.access |= newState.access;
                        oldState.stage |= newState.stage;
                        continue;
                    }
                    vkrBuffer& buffer = ResolveBuffer(access.resource);
                    if (needTransfer)
                    {
                        hadTransfer[oldState.queue] = true;
                        vkrBuffer_Transfer(
                            &buffer,
                            oldState.queue, newState.queue,
                            cmds[oldState.queue], cmds[newState.queue],
                            oldState.access, newState.access,
                            oldState.stage, newState.stage);
                    }
                    else
                    {
                        vkrBuffer_Barrier(
                            &buffer,
                            cmds[newState.queue],
                            oldState.access, newState.access,
                            oldState.stage, newState.stage);
                    }
                    oldState = newState;
                }
                break;
                case ResourceType_Image:
                {
                    ResourceState& oldState = m_images.GetState(access.resource);
                    bool needTransfer = oldState.queue != newState.queue;
                    bool needLayout = oldState.layout != newState.layout;
                    bool pendingWrites = (oldState.access & kWriteAccess) != 0x0;
                    bool pendingReads = (oldState.access & kReadAccess) != 0x0;
                    bool upcomingWrites = (newState.access & kWriteAccess) != 0x0;
                    bool upcomingReads = (newState.access & kReadAccess) != 0x0;
                    bool hazard =
                        (needTransfer) ||
                        (needLayout) ||
                        (pendingWrites && (upcomingReads || upcomingWrites)) ||
                        (pendingReads && (upcomingWrites));
                    if (!hazard)
                    {
                        oldState.access |= newState.access;
                        oldState.stage |= newState.stage;
                        continue;
                    }
                    vkrImage& image = ResolveImage(access.resource);
                    if (needTransfer)
                    {
                        hadTransfer[oldState.queue] = true;
                        vkrImage_Transfer(
                            &image,
                            oldState.queue, newState.queue,
                            cmds[oldState.queue], cmds[newState.queue],
                            newState.layout,
                            oldState.access, newState.access,
                            oldState.stage, newState.stage);
                    }
                    else
                    {
                        vkrImage_Barrier(
                            &image,
                            cmds[oldState.queue],
                            newState.layout,
                            oldState.access, newState.access,
                            oldState.stage, newState.stage);
                    }
                    oldState = newState;
                }
                break;
                }
            }

            for (i32 i = 0; i < vkrQueueId_COUNT; ++i)
            {
                if (hadTransfer[i])
                {
                    hadTransfer[i] = false;
                    vkrCmdEnd(cmds[i]);
                    vkrCmdSubmit(queues[i], cmds[i], fences[i], NULL, 0x0, NULL);
                    cmds[i] = vkrContext_GetTmpCmd(ctx, (vkrQueueId)i, &fences[i], &queues[i]);
                    vkrCmdBegin(cmds[i]);
                }
            }

            pass->Execute(cmds[pass->m_queue]);
        }

        for (i32 i = 0; i < vkrQueueId_COUNT; ++i)
        {
            vkrCmdEnd(cmds[i]);
            vkrCmdSubmit(queues[i], cmds[i], fences[i], NULL, 0x0, NULL);
        }
    }

    vkrBuffer& FrameGraph::ResolveBuffer(ResourceId id)
    {
        return m_buffers.GetPhysical(id);
    }

    vkrImage& FrameGraph::ResolveImage(ResourceId id)
    {
        return m_images.GetPhysical(id);
    }
};
