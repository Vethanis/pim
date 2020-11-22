#include "rendering/framegraph/framegraph.hpp"
#include "rendering/vulkan/vkr_buffer.h"
#include "rendering/vulkan/vkr_image.h"
#include "rendering/vulkan/vkr_context.h"
#include "rendering/vulkan/vkr_cmd.h"

namespace vkr
{
    // ------------------------------------------------------------------------
    // BufferTable

    BufferTable::~BufferTable()
    {
        for (i32 i : m_ids)
        {
            Buffer& buf = m_buffers[i];
            for (i32 j = 0; j < kFramesInFlight; ++j)
            {
                vkrBuffer_Del(&buf.frames[j]);
            }
        }
    }

    void BufferTable::Update()
    {
        const u32 curFrame = vkr_frameIndex();
        for (i32 slot : m_ids)
        {
            u32 duration = (curFrame - m_frames[slot]);
            if ((duration > kFramesInFlight))
            {
                Buffer& buf = m_buffers[slot];
                for (i32 j = 0; j < kFramesInFlight; ++j)
                {
                    vkrBuffer_Del(&buf.frames[j]);
                }
                bool freed = m_ids.FreeAt(slot);
                ASSERT(freed);
                m_names[slot] = {};
                m_infos[slot] = {};
                m_frames[slot] = {};
                m_states[slot] = {};
            }
        }
    }

    ResourceId BufferTable::Get(const BufferInfo& info, const char* name)
    {
        const Hash64 hash(name);
        ASSERT(!hash.IsNull());

        i32 slot = -1;
        const i32 len = m_names.Size();
        for (i32 i = 0; i < len; ++i)
        {
            if (m_names[i] == hash)
            {
                VkBufferUsageFlags prevUsage = m_infos[i].usage;
                m_infos[i].usage |= info.usage;
                if (m_infos[i] == info)
                {
                    slot = i;
                    break;
                }
                m_infos[i].usage = prevUsage;
            }
        }

        if (slot < 0)
        {
            GenId id = m_ids.Alloc();
            slot = id.index;
            if (slot >= len)
            {
                m_names.Grow();
                m_infos.Grow();
                m_buffers.Grow();
                m_frames.Grow();
                m_states.Grow();
            }
            m_names[slot] = hash;
            m_infos[slot] = info;
        }

        m_frames[slot] = vkr_frameIndex();

        ResourceId rid = {};
        rid.type = ResourceType_Buffer;
        rid.syncIndex = vkr_syncIndex();
        rid.id.index = slot;
        rid.id.version = m_ids.GetVersions()[slot];
        ASSERT(rid.id.version & 1);

        return rid;
    }

    void BufferTable::CreatePhysical()
    {
        for (i32 slot : m_ids)
        {
            Buffer& buffer = m_buffers[slot];
            if (!buffer.frames[0].handle)
            {
                ResourceFrameState& state = m_states[slot];
                const BufferInfo& info = m_infos[slot];
                for (i32 i = 0; i < kFramesInFlight; ++i)
                {
                    bool created = vkrBuffer_New(
                        &buffer.frames[i],
                        info.size,
                        info.usage,
                        info.memUsage,
                        PIM_FILELINE);
                    ASSERT(created);
                    state.frames[i].queue = vkrQueueId_Gfx;
                    state.frames[i].stage = VK_PIPELINE_STAGE_HOST_BIT;
                    state.frames[i].access = VK_ACCESS_HOST_WRITE_BIT;
                    state.frames[i].layout = VK_IMAGE_LAYOUT_UNDEFINED;
                }
            }
        }
    }

    vkrBuffer *const BufferTable::Resolve(ResourceId rid)
    {
        ASSERT(rid.type == ResourceType_Buffer);
        if (m_ids.Exists(rid.id))
        {
            i32 slot = rid.id.index;
            m_frames[slot] = vkr_frameIndex();
            return &m_buffers[slot].frames[rid.syncIndex];
        }
        return NULL;
    }

    const ResourceState& BufferTable::GetState(ResourceId rid) const
    {
        ASSERT(Exists(rid));
        return m_states[rid.id.index].frames[rid.syncIndex];
    }
    void BufferTable::SetState(ResourceId rid, const ResourceState& state)
    {
        ASSERT(Exists(rid));
        m_states[rid.id.index].frames[rid.syncIndex] = state;
    }

    // ------------------------------------------------------------------------
    // ImageTable

    ImageTable::~ImageTable()
    {
        for (i32 i : m_ids)
        {
            Image& img = m_images[i];
            for (i32 j = 0; j < kFramesInFlight; ++j)
            {
                vkrImage_Del(&img.frames[j]);
            }
        }
    }

    void ImageTable::Update()
    {
        const u32 curFrame = vkr_frameIndex();
        for (i32 slot : m_ids)
        {
            u32 duration = (curFrame - m_frames[slot]);
            if ((duration > kFramesInFlight))
            {
                Image& img = m_images[slot];
                for (i32 j = 0; j < kFramesInFlight; ++j)
                {
                    vkrImage_Del(&img.frames[j]);
                }
                bool freed = m_ids.FreeAt(slot);
                ASSERT(freed);
                m_names[slot] = {};
                m_infos[slot] = {};
                m_frames[slot] = {};
                m_states[slot] = {};
            }
        }
    }

    ResourceId ImageTable::Get(const ImageInfo& info, const char* name)
    {
        const Hash64 hash(name);
        ASSERT(!hash.IsNull());

        i32 slot = -1;
        const i32 len = m_names.Size();
        for (i32 i = 0; i < len; ++i)
        {
            if (m_names[i] == hash)
            {
                VkImageUsageFlags prevUsage = m_infos[i].info.usage;
                m_infos[i].info.usage |= info.info.usage;
                if (m_infos[i] == info)
                {
                    slot = i;
                    break;
                }
                m_infos[i].info.usage = prevUsage;
            }
        }

        if (slot < 0)
        {
            GenId id = m_ids.Alloc();
            slot = id.index;
            if (slot >= len)
            {
                m_names.Grow();
                m_infos.Grow();
                m_images.Grow();
                m_frames.Grow();
                m_states.Grow();
            }
            m_names[slot] = hash;
            m_infos[slot] = info;
        }

        m_frames[slot] = vkr_frameIndex();

        ResourceId rid = {};
        rid.type = ResourceType_Image;
        rid.syncIndex = vkr_syncIndex();
        rid.id.index = slot;
        rid.id.version = m_ids.GetVersions()[slot];
        ASSERT(rid.id.version & 1);

        return rid;
    }

    void ImageTable::CreatePhysical()
    {
        for (i32 slot : m_ids)
        {
            Image& image = m_images[slot];
            if (!image.frames[0].handle)
            {
                ResourceFrameState& state = m_states[slot];
                const ImageInfo& info = m_infos[slot];
                for (i32 i = 0; i < kFramesInFlight; ++i)
                {
                    bool created = vkrImage_New(
                        &image.frames[i],
                        &info.info,
                        info.memUsage,
                        PIM_FILELINE);
                    ASSERT(created);
                    state.frames[i].queue = vkrQueueId_Gfx;
                    state.frames[i].stage = VK_PIPELINE_STAGE_HOST_BIT;
                    state.frames[i].access = VK_ACCESS_HOST_WRITE_BIT;
                    state.frames[i].layout = info.info.initialLayout;
                }
            }
        }
    }

    vkrImage *const ImageTable::Resolve(ResourceId rid)
    {
        ASSERT(rid.type == ResourceType_Image);
        if (m_ids.Exists(rid.id))
        {
            i32 slot = rid.id.index;
            m_frames[slot] = vkr_frameIndex();
            return &m_images[slot].frames[rid.syncIndex];
        }
        return NULL;
    }

    const ResourceState& ImageTable::GetState(ResourceId rid) const
    {
        ASSERT(Exists(rid));
        return m_states[rid.id.index].frames[rid.syncIndex];
    }
    void ImageTable::SetState(ResourceId rid, const ResourceState& state)
    {
        ASSERT(Exists(rid));
        m_states[rid.id.index].frames[rid.syncIndex] = state;
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
        ra.state.stage = stageFlags;
        ra.state.access = accessMask;
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
        ra.state.stage = stageFlags;
        ra.state.access = accessMask;
        ra.state.layout = layout;
        return rid;
    }

    vkrBuffer *const RenderPass::ResolveBuffer(ResourceId id)
    {
        ASSERT(m_graph);
        return m_graph->ResolveBuffer(id);
    }

    vkrImage *const RenderPass::ResolveImage(ResourceId id)
    {
        ASSERT(m_graph);
        return m_graph->ResolveImage(id);
    }

    // ------------------------------------------------------------------------
    // FrameGraph

    void FrameGraph::Begin()
    {
        m_buffers.Update();
        m_images.Update();
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

    void FrameGraph::End()
    {
        m_buffers.CreatePhysical();
        m_images.CreatePhysical();

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
                const ResourceState& dstState = access.state;
                switch (access.resource.type)
                {
                default:
                    ASSERT(false);
                    break;
                case ResourceType_Buffer:
                {
                    const ResourceState& srcState = m_buffers.GetState(access.resource);
                    if (srcState != dstState)
                    {
                        vkrBuffer *const buffer = ResolveBuffer(access.resource);
                        if (srcState.queue != dstState.queue)
                        {
                            hadTransfer[srcState.queue] = true;
                            vkrBuffer_Transfer(
                                buffer,
                                srcState.queue, dstState.queue,
                                cmds[srcState.queue], cmds[dstState.queue],
                                srcState.access, dstState.access,
                                srcState.stage, dstState.stage);
                        }
                        else
                        {
                            vkrBuffer_Barrier(
                                buffer,
                                cmds[srcState.queue],
                                srcState.access, dstState.access,
                                srcState.stage, dstState.stage);
                        }
                        m_buffers.SetState(access.resource, access.state);
                    }
                }
                break;
                case ResourceType_Image:
                {
                    const ResourceState& srcState = m_images.GetState(access.resource);
                    if (srcState != dstState)
                    {
                        vkrImage *const image = ResolveImage(access.resource);
                        if (srcState.queue != dstState.queue)
                        {
                            hadTransfer[srcState.queue] = true;
                            vkrImage_Transfer(
                                image,
                                srcState.queue, dstState.queue,
                                cmds[srcState.queue], cmds[dstState.queue],
                                dstState.layout,
                                srcState.access, dstState.access,
                                srcState.stage, dstState.stage);
                        }
                        else
                        {
                            vkrImage_Barrier(
                                image,
                                cmds[srcState.queue],
                                dstState.layout,
                                srcState.access, dstState.access,
                                srcState.stage, dstState.stage);
                        }
                        m_images.SetState(access.resource, access.state);
                    }
                }
                break;
                }
            }

            for (i32 i = 0; i < vkrQueueId_COUNT; ++i)
            {
                if (hadTransfer[i])
                {
                    vkrCmdEnd(cmds[i]);
                    vkrCmdSubmit(queues[i], cmds[i], fences[i], NULL, 0x0, NULL);
                    cmds[i] = vkrContext_GetTmpCmd(ctx, (vkrQueueId)i, &fences[i], &queues[i]);
                    vkrCmdBegin(cmds[i]);
                    hadTransfer[i] = false;
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

};
