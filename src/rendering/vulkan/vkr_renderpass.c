#include "rendering/vulkan/vkr_renderpass.h"
#include "allocator/allocator.h"
#include "containers/dict.h"
#include "common/guid.h"
#include "common/profiler.h"
#include "threading/spinlock.h"
#include <string.h>

static Spinlock ms_lock;
static Dict ms_cache =
{
    .keySize = sizeof(Guid),
    .valueSize = sizeof(VkRenderPass),
};

static Guid HashRenderPass(
    i32 attachmentCount,
    const VkAttachmentDescription* pAttachments,
    i32 subpassCount,
    const VkSubpassDescription* pSubpasses,
    i32 dependencyCount,
    const VkSubpassDependency* pDependencies);
static Guid HashAttachments(const VkAttachmentDescription* attachments, i32 count);
static Guid HashSubpasses(const VkSubpassDescription* subpasses, i32 count);
static Guid HashDependencies(const VkSubpassDependency* dependencies, i32 count);

ProfileMark(pm_get, vkrRenderPass_GetFull)
VkRenderPass vkrRenderPass_GetFull(
    i32 attachmentCount,
    const VkAttachmentDescription* pAttachments,
    i32 subpassCount,
    const VkSubpassDescription* pSubpasses,
    i32 dependencyCount,
    const VkSubpassDependency* pDependencies)
{
    VkRenderPass handle = NULL;

    ProfileBegin(pm_get);

    const Guid key = HashRenderPass(
        attachmentCount, pAttachments,
        subpassCount, pSubpasses,
        dependencyCount, pDependencies);

    Spinlock_Lock(&ms_lock);
    {
        if (!Dict_Get(&ms_cache, &key, &handle))
        {
            const VkRenderPassCreateInfo createInfo =
            {
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                .attachmentCount = attachmentCount,
                .pAttachments = pAttachments,
                .subpassCount = subpassCount,
                .pSubpasses = pSubpasses,
                .dependencyCount = dependencyCount,
                .pDependencies = pDependencies,
            };
            ASSERT(g_vkr.dev);
            VkCheck(vkCreateRenderPass(g_vkr.dev, &createInfo, NULL, &handle));
            ASSERT(handle);
            Dict_Add(&ms_cache, &key, &handle);
        }
    }
    Spinlock_Unlock(&ms_lock);

    ProfileEnd(pm_get);

    ASSERT(handle);
    return handle;
}

void vkrRenderPass_Clear(void)
{
    Spinlock_Lock(&ms_lock);
    VkDevice dev = g_vkr.dev;
    VkRenderPass* passes = ms_cache.values;
    const i32 width = ms_cache.width;
    for (i32 i = 0; i < width; ++i)
    {
        VkRenderPass pass = passes[i];
        if (pass)
        {
            vkDestroyRenderPass(dev, pass, NULL);
        }
    }
    Dict_Clear(&ms_cache);
    Spinlock_Unlock(&ms_lock);
}

VkRenderPass vkrRenderPass_Get(const vkrRenderPassDesc* desc)
{
    VkAttachmentDescription attachments[9] = { 0 };
    VkAttachmentReference refs[9] = { 0 };

    i32 depthCount = 0;
    if (desc->depth.format != VK_FORMAT_UNDEFINED)
    {
        const i32 iAttach = 0;
        const vkrAttachmentState* src = &desc->depth;
        VkAttachmentDescription* dst = &attachments[iAttach];
        dst->format = src->format;
        dst->samples = VK_SAMPLE_COUNT_1_BIT;
        dst->loadOp = src->load;
        dst->storeOp = src->store;
        dst->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        dst->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        dst->initialLayout = src->initialLayout;
        dst->finalLayout = src->finalLayout;
        refs[iAttach].attachment = iAttach;
        refs[iAttach].layout = src->layout;
        ++depthCount;
    }

    i32 colorCount = 0;
    for (i32 i = 0; i < NELEM(desc->color); ++i)
    {
        const vkrAttachmentState* src = &desc->color[i];
        if (src->format != VK_FORMAT_UNDEFINED)
        {
            const i32 iAttach = depthCount + colorCount;
            VkAttachmentDescription* dst = &attachments[iAttach];
            dst->format = src->format;
            dst->samples = VK_SAMPLE_COUNT_1_BIT;
            dst->loadOp = src->load;
            dst->storeOp = src->store;
            dst->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            dst->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            dst->initialLayout = src->initialLayout;
            dst->finalLayout = src->finalLayout;
            refs[iAttach].attachment = iAttach;
            refs[iAttach].layout = src->layout;
            ++colorCount;
        }
    }

    const VkSubpassDescription subpass =
    {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = colorCount,
        .pColorAttachments = colorCount ? &refs[depthCount] : NULL,
        .pDepthStencilAttachment = depthCount ? &refs[0] : NULL,
    };

    const VkSubpassDependency dependency =
    {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = desc->srcStageMask,
        .dstStageMask = desc->dstStageMask,
        .srcAccessMask = desc->srcAccessMask,
        .dstAccessMask = desc->dstAccessMask,
    };

    return vkrRenderPass_GetFull(depthCount + colorCount, attachments, 1, &subpass, 1, &dependency);
}

// ----------------------------------------------------------------------------

static Guid HashRenderPass(
    i32 attachmentCount,
    const VkAttachmentDescription* pAttachments,
    i32 subpassCount,
    const VkSubpassDescription* pSubpasses,
    i32 dependencyCount,
    const VkSubpassDependency* pDependencies)
{
    const Guid keys[] =
    {
        HashAttachments(pAttachments, attachmentCount),
        HashSubpasses(pSubpasses, subpassCount),
        HashDependencies(pDependencies, dependencyCount),
    };
    return Guid_FromBytes(keys, sizeof(keys));
}

static Guid HashAttachments(const VkAttachmentDescription* attachments, i32 count)
{
    return Guid_FromBytes(attachments, sizeof(attachments[0]) * count);
}

static Guid HashSubpasses(const VkSubpassDescription* subpasses, i32 count)
{
    Guid hash = { 0 };

    for (i32 i = 0; i < count; ++i)
    {
        const Guid keys[] =
        {
            hash,
            Guid_FromBytes(&subpasses[i].flags, sizeof(subpasses[i].flags)),
            Guid_FromBytes(&subpasses[i].pipelineBindPoint, sizeof(subpasses[i].pipelineBindPoint)),
            Guid_FromBytes(
                subpasses[i].pInputAttachments,
                sizeof(subpasses[i].pInputAttachments[0]) * subpasses[i].inputAttachmentCount),
            Guid_FromBytes(
                subpasses[i].pColorAttachments,
                sizeof(subpasses[i].pColorAttachments[0]) * subpasses[i].colorAttachmentCount),
            Guid_FromBytes(
                subpasses[i].pResolveAttachments,
                sizeof(subpasses[i].pResolveAttachments[0]) * subpasses[i].colorAttachmentCount),
            Guid_FromBytes(
                subpasses[i].pDepthStencilAttachment,
                sizeof(subpasses[i].pDepthStencilAttachment[0])),
            Guid_FromBytes(
                subpasses[i].pPreserveAttachments,
                sizeof(subpasses[i].pPreserveAttachments[0]) * subpasses[i].preserveAttachmentCount),
        };
        hash = Guid_FromBytes(keys, sizeof(keys));
    }

    return hash;
}

static Guid HashDependencies(const VkSubpassDependency* dependencies, i32 count)
{
    return Guid_FromBytes(dependencies, sizeof(dependencies[0]) * count);
}
