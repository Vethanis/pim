#include "rendering/vulkan/vkr_rendergraph.hpp"

namespace vkr
{
    static void AddAccess(Array<ResourceAccess>& set, const ResourceAccess& access)
    {
        ASSERT(access.resource);
        for (ResourceAccess& ra : set)
        {
            if (ra.resource == access.resource)
            {
                // can't read two layouts in same pass
                // can't write two layouts in same pass
                ASSERT(ra.layout == access.layout);
                ra.access |= access.access;
                ra.stage |= access.stage;
                return;
            }
        }
        set.Grow() = access;
    }

    // ------------------------------------------------------------------------

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
        ResourceAccess ra = {};
        ra.resource = &ir;
        ra.stage = stage;
        ra.access = access;
        ra.layout = layout;
        AddAccess(m_inputs, ra);
        return ir;
    }

    ImageResource& RenderPass::AddImageOutput(
        const char* name,
        VkPipelineStageFlags stage,
        VkAccessFlags access,
        VkImageLayout layout,
        VkImageUsageFlags usage,
        const AttachmentInfo& info)
    {
        ImageResource& ir = m_graph.GetImageResource(name);
        ir.SetAttachmentInfo(info);
        ir.AddQueue(m_queueFlags);
        ir.WriteInPass(m_id);
        ir.AddImageUsage(usage);
        ResourceAccess ra = {};
        ra.resource = &ir;
        ra.stage = stage;
        ra.access = access;
        ra.layout = layout;
        AddAccess(m_outputs, ra);
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
        ResourceAccess ra = {};
        ra.resource = &br;
        ra.stage = stage;
        ra.access = access;
        AddAccess(m_inputs, ra);
        return br;
    }

    BufferResource& RenderPass::AddBufferOutput(
        const char* name,
        VkPipelineStageFlags stage,
        VkAccessFlags access,
        VkBufferUsageFlags usage,
        const BufferInfo& info)
    {
        BufferResource& br = m_graph.GetBufferResource(name);
        br.SetBufferInfo(info);
        br.AddQueue(m_queueFlags);
        br.ReadInPass(m_id);
        br.AddBufferUsage(usage);
        ResourceAccess ra = {};
        ra.resource = &br;
        ra.stage = stage;
        ra.access = access;
        AddAccess(m_outputs, ra);
        return br;
    }

    // ------------------------------------------------------------------------

    void ProviderMap::Register(IResourceProvider* provider)
    {
        ASSERT(provider);
        ASSERT(!m_providers.Contains(provider));
        m_providers.Grow() = provider;
    }

    void ProviderMap::Update()
    {
        for (IResourceProvider* provider : m_providers)
        {
            provider->Update();
        }
    }

    PhysicalResource* ProviderMap::Resolve(const Resource* resource)
    {
        ASSERT(resource);
        for (IResourceProvider* provider : m_providers)
        {
            PhysicalResource* physical = provider->Resolve(resource);
            if (physical)
            {
                return physical;
            }
        }
        return NULL;
    }

    // ------------------------------------------------------------------------

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
