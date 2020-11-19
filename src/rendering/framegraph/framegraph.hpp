#pragma once

#include "common/macro.h"
#include "containers/array.hpp"
#include "containers/queue.hpp"
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
        ResourceType type;
        u8 syncIndex;
        u16 slot;
    };
    SASSERT(sizeof(ResourceId) <= 4);

    struct ResourceAccess
    {
        ResourceId resource;
        RenderPass* pass;
        VkPipelineStageFlags stage;
        VkAccessFlags access;
        VkImageLayout layout;
        vkrQueueId queue;
    };

    class ResourceTable
    {
    private:
        Queue<u16> m_freeBuffers;
        Queue<u16> m_freeImages;
        Array<vkrBuffer> m_buffers[kFramesInFlight];
        Array<vkrImage> m_images[kFramesInFlight];
    public:
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
