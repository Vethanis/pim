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
        Hash64 name;
        ResourceType type;
        u8 syncIndex;
    };

    struct ResourceState
    {
        vkrQueueId queue = vkrQueueId_Gfx;
        VkPipelineStageFlags stage = 0x0;
        VkAccessFlags access = 0x0;
        VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
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

        void Merge(const BufferInfo& other);
    };

    struct ImageInfo
    {
        VkImageCreateInfo info;
        vkrMemUsage memUsage;

        void Merge(const ImageInfo& other);
    };

    struct Buffer
    {
        vkrBuffer frames[kFramesInFlight];

        bool Create(const BufferInfo& info);
        void Release();
    };

    struct Image
    {
        vkrImage frames[kFramesInFlight];

        bool Create(const ImageInfo& info);
        void Release();
    };

    class BufferTable final
    {
    private:
        Array<Hash64> m_logicalNames;
        Array<BufferInfo> m_logicalInfos;

        Array<Hash64> m_names;
        Array<BufferInfo> m_infos;
        Array<Buffer> m_buffers;
        Array<u32> m_frames;
        Array<ResourceFrameState> m_states;
    public:
        BufferTable() {}
        ~BufferTable();

        void BeforeSetup();
        ResourceId GetLogical(const BufferInfo& info, const char* name);

        void BeforeExecute();
        vkrBuffer& GetPhysical(ResourceId rid);
        ResourceState& GetState(ResourceId rid);

    private:
        i32 Alloc(const BufferInfo& info, Hash64 name);
        void Free(i32 slot);
    };

    class ImageTable final
    {
    private:
        Array<Hash64> m_logicalNames;
        Array<ImageInfo> m_logicalInfos;

        Array<Hash64> m_names;
        Array<ImageInfo> m_infos;
        Array<Image> m_images;
        Array<u32> m_frames;
        Array<ResourceFrameState> m_states;
    public:
        ImageTable() {}
        ~ImageTable();

        void BeforeSetup();
        ResourceId GetLogical(const ImageInfo& info, const char* name);

        void BeforeExecute();
        vkrImage& GetPhysical(ResourceId rid);
        ResourceState& GetState(ResourceId rid);

    private:
        i32 Alloc(const ImageInfo& info, Hash64 name);
        void Free(i32 slot);
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

        vkrBuffer& ResolveBuffer(ResourceId id);
        vkrImage& ResolveImage(ResourceId id);

        ResourceId ReadVertexBuffer(const BufferInfo& info, const char* name);
        ResourceId ReadIndexBuffer(const BufferInfo& info, const char* name);
        ResourceId ReadIndirectBuffer(const BufferInfo& info, const char* name);
        ResourceId ReadUniformBuffer(
            const BufferInfo& info, const char* name, VkPipelineStageFlags stages);

        ResourceId ReadStorageBuffer(
            const BufferInfo& info, const char* name, VkPipelineStageFlags stages);
        ResourceId WriteStorageBuffer(
            const BufferInfo& info, const char* name, VkPipelineStageFlags stages);
        ResourceId ReadWriteStorageBuffer(
            const BufferInfo& info, const char* name, VkPipelineStageFlags stages);

        ResourceId InputColorAttachment(const ImageInfo& info, const char* name);
        ResourceId OutputColorAttachment(const ImageInfo& info, const char* name);

        ResourceId InputDepthAttachment(const ImageInfo& info, const char* name);
        ResourceId OutputDepthAttachment(const ImageInfo& info, const char* name);

        ResourceId SampleImage(
            const ImageInfo& info, const char* name, VkPipelineStageFlags stages);

        ResourceId ReadStorageImage(
            const ImageInfo& info, const char* name, VkPipelineStageFlags stages);
        ResourceId WriteStorageImage(
            const ImageInfo& info, const char* name, VkPipelineStageFlags stages);
        ResourceId ReadWriteStorageImage(
            const ImageInfo& info, const char* name, VkPipelineStageFlags stages);

        ResourceId PresentImage(const ImageInfo& info, const char* name);
    };

    class FrameGraph final
    {
    private:
        BufferTable m_buffers;
        ImageTable m_images;
        Array<RenderPass*> m_passes;
    public:
        // --------------------------------------------------------------------
        // Setup
        void Begin();
        void AddPass(RenderPass* pass);
        ResourceId GetBuffer(const BufferInfo& info, const char* name);
        ResourceId GetImage(const ImageInfo& info, const char* name);

        // --------------------------------------------------------------------
        // Execute
        void End();
        vkrBuffer& ResolveBuffer(ResourceId id);
        vkrImage& ResolveImage(ResourceId id);
    };
};
