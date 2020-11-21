#pragma once

#include "common/macro.h"
#include "containers/array.hpp"
#include "containers/queue.hpp"
#include "containers/hash64.hpp"
#include "containers/gen_id.hpp"
#include "containers/id_alloc.hpp"
#include "containers/mem.hpp"
#include "common/time.h"
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
        u8 frame;
    };

    struct ResourceAccess
    {
        ResourceId resource;
        RenderPass* pass;
        VkPipelineStageFlags stage;
        VkAccessFlags access;
        VkImageLayout layout;
        vkrQueueId queue;
    };

    struct BufferInfo
    {
        i32 size;
        VkBufferUsageFlags usage;
    };

    struct ImageInfo
    {
        VkImageType type;
        VkFormat format;
        VkImageUsageFlags usage;
        u16 width;
        u16 height;
        u16 depth;
        u8 mipLevels;
        u8 arrayLayers;
    };

    struct Buffer
    {
        vkrBuffer buffers[kFramesInFlight];
    };

    struct Image
    {
        vkrImage images[kFramesInFlight];
    };

    class BufferTable final
    {
    private:
        IdAllocator m_ids;
        Array<Hash64> m_names;
        Array<BufferInfo> m_infos;
        Array<Buffer> m_buffers;
        Array<u32> m_frames;
    public:
        BufferTable() {}
        ~BufferTable();

        void Update();
        GenId Alloc(const BufferInfo& info, const char* name);
        bool Free(GenId id);

        i32 Size() const { return m_names.Size(); }
        bool Exists(GenId id) const { return m_ids.Exists(id); }
        Buffer *const Get(GenId id)
        {
            if (Exists(id))
            {
                i32 index = id.index;
                m_frames[index] = time_framecount();
                return &m_buffers[index];
            }
            return NULL;
        }
    };

    class ImageTable final
    {
    private:
        IdAllocator m_ids;
        Array<Hash64> m_names;
        Array<ImageInfo> m_infos;
        Array<Image> m_images;
        Array<u32> m_frames;
    public:
        ImageTable() {}
        ~ImageTable();

        void Update();
        GenId Alloc(const ImageInfo& info, const char* name);
        bool Free(GenId id);

        i32 Size() const { return m_names.Size(); }
        bool Exists(GenId id) const { return m_ids.Exists(id); }
        Image *const Get(GenId id)
        {
            if (Exists(id))
            {
                i32 index = id.index;
                m_frames[index] = time_framecount();
                return &m_images[index];
            }
            return NULL;
        }
    };

    class ResourceTable
    {
    private:
        BufferTable m_buffers;
        ImageTable m_images;
    public:

        void Update()
        {
            m_buffers.Update();
            m_images.Update();
        }

        GenId AllocBuffer(const BufferInfo& info, const char* name) { return m_buffers.Alloc(info, name); }
        bool FreeBuffer(GenId id) { return m_buffers.Free(id); }
        bool BufferExists(GenId id) const { return m_buffers.Exists(id); }
        Buffer *const GetBuffer(GenId id) { return m_buffers.Get(id); }

        GenId AllocImage(const ImageInfo& info, const char* name) { return m_images.Alloc(info, name); }
        bool FreeImage(GenId id) { return m_images.Free(id); }
        bool ImageExists(GenId id) const { return m_images.Exists(id); }
        Image *const GetImage(GenId id) { return m_images.Get(id); }
    };

    class RenderPass
    {
    private:
        FrameGraph& m_graph;
        Array<ResourceAccess> m_accesses;
        vkrQueueId m_queue;
    public:
    };
};
