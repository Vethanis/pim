#pragma once

#include "common/macro.h"
#include "containers/array.hpp"
#include "containers/dict.hpp"
#include "containers/hash32.hpp"
#include "rendering/vulkan/vkr.h"
#include <string.h>

namespace vkr
{
    class RenderGraph;
    class RenderPass;
    class Resource;
    class PhysicalResource;
    class ResourceProvider;

    using PassId = Hash32;
    using ResourceId = Hash32;

    enum ResourceType
    {
        ResourceType_NULL = 0,

        ResourceType_Buffer,
        ResourceType_Image,
    };

    enum QueueFlagBit
    {
        QueueFlagBit_Graphics = 1 << 0,
        QueueFlagBit_Compute = 1 << 1,
        QueueFlagBit_Transfer = 1 << 2,
        QueueFlagBit_Present = 1 << 3,
    };
    using QueueFlags = u32;

    enum AttachmentScaling
    {
        AttachmentScaling_DisplayRelative,
        AttachmentScaling_Absolute,
        AttachmentScaling_InputRelative,
    };

    struct BufferInfo
    {
        i32 size = 0;
        VkBufferUsageFlags usage = 0x0;

        bool operator==(const BufferInfo& other) const
        {
            return memcmp(this, &other, sizeof(*this)) == 0;
        }
    };

    struct AttachmentInfo
    {
        VkFormat format = VK_FORMAT_UNDEFINED;
        VkImageUsageFlags usage = 0x0;
        AttachmentScaling scaling = AttachmentScaling_DisplayRelative;
        Hash32 scaleName;
        float scaleX = 1.0f;
        float scaleY = 1.0f;
        float scaleZ = 1.0f;
        u8 levels = 1;
        u8 layers = 1;

        bool operator==(const AttachmentInfo& other) const
        {
            return memcmp(this, &other, sizeof(*this)) == 0;
        }
    };

    struct ImageInfo
    {
        VkFormat format = VK_FORMAT_UNDEFINED;
        VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
        VkImageUsageFlags usage = 0x0;
        u16 width = 0;
        u16 height = 0;
        u16 depth = 1;
        u8 layers = 1;
        u8 levels = 1;

        bool operator==(const ImageInfo& other) const
        {
            return memcmp(this, &other, sizeof(*this)) == 0;
        }
    };

    struct ResourceAccess
    {
        Resource* resource = NULL;
        VkPipelineStageFlags stage = 0x0;
        VkAccessFlags access = 0x0;
        VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;

        bool operator==(const ResourceAccess& other) const
        {
            return memcmp(this, &other, sizeof(*this)) == 0;
        }
    };

    class Resource
    {
    protected:
        ResourceType m_type;
        ResourceId m_id;
        Array<PassId> m_writePasses;
        Array<PassId> m_readPasses;
        QueueFlags m_queueFlags;
    public:
        explicit Resource(ResourceType type, ResourceId id) :
            m_type(type),
            m_id(id),
            m_writePasses(EAlloc_Temp),
            m_readPasses(EAlloc_Temp),
            m_queueFlags(0x0)
        {}
        virtual ~Resource() = default;

        ResourceType GetType() const { return m_type; }
        ResourceId GetId() const { return m_id; }

        void WriteInPass(PassId id) { m_writePasses.FindAdd(id); }
        void ReadInPass(PassId id) { m_readPasses.FindAdd(id); }
        const Array<PassId>& GetWritePasses() const { return m_writePasses; }
        const Array<PassId>& GetReadPasses() const { return m_readPasses; }

        QueueFlags GetQueues() const { return m_queueFlags; }
        void AddQueue(QueueFlags queue) { m_queueFlags |= queue; }
    };

    class BufferResource : public Resource
    {
    protected:
        BufferInfo m_info;
    public:
        explicit BufferResource(ResourceId id)
            : Resource(ResourceType_Buffer, id)
        {}

        void SetBufferInfo(const BufferInfo& info) { m_info = info; }
        const BufferInfo& GetBufferInfo() const { return m_info; }

        VkBufferUsageFlags GetBufferUsage() const { return m_info.usage; }
        void AddBufferUsage(VkBufferUsageFlags usage) { m_info.usage |= usage; }
    };

    class ImageResource : public Resource
    {
    protected:
        AttachmentInfo m_info;
    public:
        explicit ImageResource(ResourceId id)
            : Resource(ResourceType_Image, id)
        {}

        const AttachmentInfo& GetAttachmentInfo() const { return m_info; }
        void SetAttachmentInfo(const AttachmentInfo& info) { m_info = info; }

        VkImageUsageFlags GetImageUsage() const { return m_info.usage; }
        void AddImageUsage(VkImageUsageFlags usage) { m_info.usage |= usage; }
    };

    class RenderPass
    {
    private:
        RenderGraph& m_graph;
        PassId m_id;
        QueueFlags m_queueFlags;
        Array<ResourceAccess> m_inputs;
        Array<ResourceAccess> m_outputs;
    public:
        explicit RenderPass(
            RenderGraph& graph,
            PassId id,
            QueueFlags queueFlags) :
            m_graph(graph),
            m_id(id),
            m_queueFlags(queueFlags),
            m_inputs(EAlloc_Temp),
            m_outputs(EAlloc_Temp)
        {}
        virtual ~RenderPass() = default;

        virtual bool Setup() = 0;
        virtual void Execute() = 0;

        PassId GetId() const { return m_id; }

        const Array<ResourceAccess>& GetInputs() const { return m_inputs; }
        const Array<ResourceAccess>& GetOutputs() const { return m_outputs; }

        ImageResource& AddImageInput(
            const char* name,
            VkPipelineStageFlags stage,
            VkAccessFlags access,
            VkImageLayout layout,
            VkImageUsageFlags usage);
        ImageResource& AddImageOutput(
            const char* name,
            VkPipelineStageFlags stage,
            VkAccessFlags access,
            VkImageLayout layout,
            VkImageUsageFlags usage,
            const AttachmentInfo& info);

        BufferResource& AddBufferInput(
            const char* name,
            VkPipelineStageFlags stage,
            VkAccessFlags access,
            VkBufferUsageFlags usage);
        BufferResource& AddBufferOutput(
            const char* name,
            VkPipelineStageFlags stage,
            VkAccessFlags access,
            VkBufferUsageFlags usage,
            const BufferInfo& info);

        ImageResource& ReadColor(const char* name)
        {
            return AddImageInput(
                name,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VK_ACCESS_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
        }
        ImageResource& EmitColor(const char* name, const AttachmentInfo& info)
        {
            return AddImageOutput(
                name,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                info);
        }

        ImageResource& ReadDepth(const char* name)
        {
            return AddImageInput(
                name,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VK_ACCESS_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
        }
        ImageResource& EmitDepth(const char* name, const AttachmentInfo& info)
        {
            return AddImageOutput(
                name,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                info);
        }

        ImageResource& SampleTexture(const char* name, VkPipelineStageFlags stage)
        {
            return AddImageInput(
                name,
                stage,
                VK_ACCESS_SHADER_READ_BIT,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_IMAGE_USAGE_SAMPLED_BIT);
        }

        ImageResource& ReadStorageImage(const char* name, VkPipelineStageFlags stage)
        {
            return AddImageInput(
                name,
                stage,
                VK_ACCESS_SHADER_READ_BIT,
                VK_IMAGE_LAYOUT_GENERAL,
                VK_IMAGE_USAGE_STORAGE_BIT);
        }
        ImageResource& WriteStorageImage(
            const char* name,
            VkPipelineStageFlags stage,
            const AttachmentInfo& info)
        {
            return AddImageOutput(
                name,
                stage,
                VK_ACCESS_SHADER_WRITE_BIT,
                VK_IMAGE_LAYOUT_GENERAL,
                VK_IMAGE_USAGE_STORAGE_BIT,
                info);
        }

        BufferResource& ReadVertex(const char* name)
        {
            return AddBufferInput(
                name,
                VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
                VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        }
        BufferResource& ReadIndex(const char* name)
        {
            return AddBufferInput(
                name,
                VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
                VK_ACCESS_INDEX_READ_BIT,
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
        }
        BufferResource& ReadIndirect(const char* name)
        {
            return AddBufferInput(
                name,
                VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
                VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
                VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);
        }

        BufferResource& ReadUniform(const char* name, VkPipelineStageFlags stage)
        {
            return AddBufferInput(
                name,
                stage,
                VK_ACCESS_UNIFORM_READ_BIT,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
        }

        BufferResource& ReadStorageBuffer(const char* name, VkPipelineStageFlags stage)
        {
            return AddBufferInput(
                name,
                stage,
                VK_ACCESS_SHADER_READ_BIT,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        }
        BufferResource& WriteStorageBuffer(
            const char* name,
            VkPipelineStageFlags stage,
            const BufferInfo& info)
        {
            return AddBufferOutput(
                name,
                stage,
                VK_ACCESS_SHADER_WRITE_BIT,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                info);
        }
    };

    class PhysicalResource
    {
    private:
        u32 m_frame; // frame of last access
        VkFence m_fence; // fence of most recent usage submit
    public:
        PhysicalResource() :
            m_frame(0),
            m_fence(NULL)
        {}
        virtual ~PhysicalResource() = default;
    };

    class PhysicalBuffer : public PhysicalResource
    {
    private:
        vkrBuffer m_buffer;
        VkBufferUsageFlags m_usage;
    public:
        explicit PhysicalBuffer(const BufferInfo& info);
        ~PhysicalBuffer();
    };

    class PhysicalImage : public PhysicalResource
    {
    private:
        vkrImage m_image;
    public:
        explicit PhysicalImage(const ImageInfo& info);
        ~PhysicalImage();
    };

    class ResourceProvider
    {
    private:
    public:
        ResourceProvider();
        virtual ~ResourceProvider() = default;

        virtual void Update() = 0;
        virtual PhysicalResource* Resolve(const Resource* resource) = 0;
    };

    class RenderGraph final
    {
    private:
        Array<PassId> m_passIds;
        Array<RenderPass*> m_passes;
        Array<ResourceId> m_resourceIds;
        Array<Resource*> m_resources;
    public:
        RenderGraph() :
            m_passIds(EAlloc_Temp),
            m_passes(EAlloc_Temp),
            m_resourceIds(EAlloc_Temp),
            m_resources(EAlloc_Temp)
        {}

        ImageResource& GetImageResource(const char* name);
        BufferResource& GetBufferResource(const char* name);

        template<class PassType>
        PassType& AddPass(const char* name, QueueFlags queueFlags)
        {
            ResourceId id(name);
            i32 index = m_passIds.Find(id);
            if (index >= 0)
            {
                return *static_cast<PassType*>(m_passes[index]);
            }
            PassType* ptr = new(tmp_calloc(sizeof(*ptr))) PassType(*this, id, queueFlags);
            m_passIds.Grow() = id;
            m_passes.Grow() = ptr;
            return *ptr;
        }
    };
};
