#pragma once

#include "common/macro.h"
#include "containers/array.hpp"
#include "containers/queue.hpp"
#include "containers/hash64.hpp"
#include "containers/gen_id.hpp"
#include "containers/id_alloc.hpp"
#include "containers/mem.hpp"
#include "rendering/vulkan/vkr.h"

namespace vkr
{
    class FrameGraph;
    class RenderPass;

    enum ResourceType : u8
    {
        ResourceType_Buffer,
        ResourceType_Image,

        ResourceType_COUNT
    };

    struct ResourceId
    {
        GenId id;
        ResourceType type;
        u8 syncIndex;
    };

    struct ResourceState
    {
        vkrQueueId queue;
        VkPipelineStageFlags stage;
        VkAccessFlags access;
        VkImageLayout layout;
    };

    struct ResourceFrameState
    {
        ResourceState frames[kFramesInFlight];
    };

    struct ResourceAccess
    {
        ResourceId resource;
        ResourceState state;
    };

    struct BufferInfo
    {
        i32 size;
        VkBufferUsageFlags usage;
        vkrMemUsage memUsage;
    };

    struct ImageInfo
    {
        VkImageCreateInfo info;
        vkrMemUsage memUsage;
    };

    struct Buffer
    {
        vkrBuffer frames[kFramesInFlight];
    };

    struct Image
    {
        vkrImage frames[kFramesInFlight];
    };

    class BufferTable final
    {
    private:
        IdAllocator m_ids;
        Array<Hash64> m_names;
        Array<BufferInfo> m_infos;
        Array<Buffer> m_buffers;
        Array<u32> m_frames;
        Array<ResourceFrameState> m_states;
    public:
        BufferTable() {}
        ~BufferTable();

        void Update();
        ResourceId Get(const BufferInfo& info, const char* name);
        void CreatePhysical();
        vkrBuffer *const Resolve(ResourceId rid);

        bool Exists(ResourceId rid) const { return m_ids.Exists(rid.id); }

        const ResourceState& GetState(ResourceId rid) const;
        void SetState(ResourceId rid, const ResourceState& state);
    };

    class ImageTable final
    {
    private:
        IdAllocator m_ids;
        Array<Hash64> m_names;
        Array<ImageInfo> m_infos;
        Array<Image> m_images;
        Array<u32> m_frames;
        Array<ResourceFrameState> m_states;
    public:
        ImageTable() {}
        ~ImageTable();

        void Update();
        ResourceId Get(const ImageInfo& info, const char* name);
        void CreatePhysical();
        vkrImage *const Resolve(ResourceId rid);

        bool Exists(ResourceId rid) const { return m_ids.Exists(rid.id); }

        const ResourceState& GetState(ResourceId rid) const;
        void SetState(ResourceId rid, const ResourceState& state);
    };

    class RenderPass
    {
        friend class FrameGraph;
    private:
        Hash64 m_name;
        vkrQueueId m_queue;
        Array<ResourceAccess> m_accesses;
        FrameGraph* m_graph;

    public:
        explicit RenderPass(const char* name, vkrQueueId queue) :
            m_name(name),
            m_queue(queue),
            m_graph(NULL)
        {
        }
        virtual ~RenderPass() {}

        virtual bool Setup() = 0;
        virtual void Execute(VkCommandBuffer cmd) = 0;

        ResourceId GetBuffer(
            const BufferInfo& info,
            const char* name,
            VkPipelineStageFlags stageFlags,
            VkAccessFlags accessMask);

        ResourceId GetImage(
            const ImageInfo& info,
            const char* name,
            VkPipelineStageFlags stageFlags,
            VkAccessFlags accessMask,
            VkImageLayout layout);

        vkrBuffer *const ResolveBuffer(ResourceId id);
        vkrImage *const ResolveImage(ResourceId id);

        ResourceId ReadVertexBuffer(
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

        ResourceId ReadIndexBuffer(
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

        ResourceId ReadIndirectBuffer(
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

        ResourceId ReadUniformBuffer(
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

        ResourceId ReadStorageBuffer(
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
        ResourceId WriteStorageBuffer(
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
        ResourceId ReadWriteStorageBuffer(
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

        ResourceId InputColorAttachment(
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
        ResourceId OutputColorAttachment(
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

        ResourceId InputDepthAttachment(
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
        ResourceId OutputDepthAttachment(
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

        ResourceId SampleImage(
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

        ResourceId ReadStorageImage(
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
        ResourceId WriteStorageImage(
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
        ResourceId ReadWriteStorageImage(
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

        ResourceId PresentImage(
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
    };

    class FrameGraph final
    {
    private:
        BufferTable m_buffers;
        ImageTable m_images;
        Array<RenderPass*> m_passes;
    public:

        void Begin();

        // --------------------------------------------------------------------
        // Setup

        void AddPass(RenderPass* pass);

        ResourceId GetBuffer(const BufferInfo& info, const char* name)
        {
            return m_buffers.Get(info, name);
        }
        ResourceId GetImage(const ImageInfo& info, const char* name)
        {
            return m_images.Get(info, name);
        }

        // --------------------------------------------------------------------

        void End();

        // --------------------------------------------------------------------
        // Execute

        vkrBuffer *const ResolveBuffer(ResourceId id)
        {
            return m_buffers.Resolve(id);
        }
        vkrImage *const ResolveImage(ResourceId id)
        {
            return m_images.Resolve(id);
        }

        // --------------------------------------------------------------------
    };
};
