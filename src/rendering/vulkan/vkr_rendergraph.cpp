#include "rendering/vulkan/vkr_rendergraph.hpp"

namespace vkr
{
    ImageResource& RenderPass::AddImageInput(
        const char* name,
        VkPipelineStageFlags stage,
        VkAccessFlags access,
        VkImageLayout layout,
        VkImageUsageFlags usage)
    {
        ImageResource& ir = m_graph.GetImageResource(name);
        ir.AddQueue(m_queueFlags);
        ir.ReadInPass(m_id);
        ir.AddImageUsage(usage);
        ResourceAccess& ra = m_inputs.Grow();
        ra.resource = &ir;
        ra.stage = stage;
        ra.access = access;
        ra.layout = layout;
        return ir;
    }

    ImageResource& RenderPass::AddImageOutput(
        const char* name,
        VkPipelineStageFlags stage,
        VkAccessFlags access,
        VkImageLayout layout,
        const AttachmentInfo& info)
    {
        ImageResource& ir = m_graph.GetImageResource(name);
        ir.SetAttachmentInfo(info);
        ir.AddQueue(m_queueFlags);
        ir.WriteInPass(m_id);
        ResourceAccess& ra = m_outputs.Grow();
        ra.resource = &ir;
        ra.stage = stage;
        ra.access = access;
        ra.layout = layout;
        return ir;
    }

    BufferResource& RenderPass::AddBufferInput(
        const char* name,
        VkPipelineStageFlags stage,
        VkAccessFlags access,
        VkBufferUsageFlags usage)
    {
        BufferResource& br = m_graph.GetBufferResource(name);
        br.AddQueue(m_queueFlags);
        br.ReadInPass(m_id);
        br.AddBufferUsage(usage);
        ResourceAccess& ra = m_inputs.Grow();
        ra.resource = &br;
        ra.stage = stage;
        ra.access = access;
        ra.layout = VK_IMAGE_LAYOUT_GENERAL;
        return br;
    }

    BufferResource& RenderPass::AddBufferOutput(
        const char* name,
        VkPipelineStageFlags stage,
        VkAccessFlags access,
        const BufferInfo& info)
    {
        BufferResource& br = m_graph.GetBufferResource(name);
        br.SetBufferInfo(info);
        br.AddQueue(m_queueFlags);
        br.ReadInPass(m_id);
        ResourceAccess& ra = m_outputs.Grow();
        ra.resource = &br;
        ra.stage = stage;
        ra.access = access;
        ra.layout = VK_IMAGE_LAYOUT_GENERAL;
        return br;
    }

    ImageResource& RenderGraph::GetImageResource(const char* name)
    {
        ResourceId id(name);
        i32 index = m_resourceIds.Find(id);
        if (index >= 0)
        {
            return *static_cast<ImageResource*>(m_resources[index]);
        }
        ImageResource* ptr = new (tmp_calloc(sizeof(*ptr))) ImageResource(id);
        m_resourceIds.Grow() = id;
        m_resources.Grow() = ptr;
        return *ptr;
    }

    BufferResource& RenderGraph::GetBufferResource(const char* name)
    {
        ResourceId id(name);
        i32 index = m_resourceIds.Find(id);
        if (index >= 0)
        {
            return *static_cast<BufferResource*>(m_resources[index]);
        }
        BufferResource* ptr = new(tmp_calloc(sizeof(*ptr))) BufferResource(id);
        m_resourceIds.Grow() = id;
        m_resources.Grow() = ptr;
        return *ptr;
    }
};
