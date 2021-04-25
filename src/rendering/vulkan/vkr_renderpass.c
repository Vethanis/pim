#include "rendering/vulkan/vkr_renderpass.h"
#include "allocator/allocator.h"
#include "containers/dict.h"
#include "common/guid.h"
#include "common/profiler.h"
#include "threading/task.h"
#include <string.h>

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
    ProfileBegin(pm_get);

    ASSERT(Task_ThreadId() == 0);

    const Guid key = HashRenderPass(
        attachmentCount, pAttachments,
        subpassCount, pSubpasses,
        dependencyCount, pDependencies);

    VkRenderPass handle = NULL;
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

    ProfileEnd(pm_get);

    ASSERT(handle);
    return handle;
}

void vkrRenderPass_Clear(void)
{
    ASSERT(Task_ThreadId() == 0);
    ASSERT(g_vkr.dev);

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
}

VkRenderPass vkrRenderPass_Get(const vkrRenderPassDesc* desc)
{
    VkAttachmentDescription attachments[8] = { 0 };
    VkAttachmentReference refs[8] = { 0 };
    i32 attachmentCount = 0;
    i32 refCount = 0;
    VkFormat format0 = desc->attachments[0].format;
    bool zeroIsDepth = (format0 >= VK_FORMAT_D16_UNORM) && (format0 <= VK_FORMAT_D32_SFLOAT_S8_UINT);

    for (i32 i = 0; i < NELEM(desc->attachments); ++i)
    {
        attachments[i].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[i].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        const vkrAttachmentState* src = &desc->attachments[i];
        if (src->format != VK_FORMAT_UNDEFINED)
        {
            ASSERT(attachmentCount < NELEM(attachments));
            VkAttachmentDescription* dst = &attachments[attachmentCount];
            dst->format = src->format;
            dst->initialLayout = src->initialLayout;
            dst->finalLayout = src->finalLayout;
            dst->samples = VK_SAMPLE_COUNT_1_BIT;
            dst->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            dst->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            dst->loadOp = src->load;
            dst->storeOp = src->store;
            refs[attachmentCount].attachment = VK_ATTACHMENT_UNUSED;
            refs[attachmentCount].layout = VK_IMAGE_LAYOUT_UNDEFINED;
            if ((dst->loadOp != VK_ATTACHMENT_LOAD_OP_DONT_CARE) ||
                (dst->storeOp != VK_ATTACHMENT_LOAD_OP_DONT_CARE))
            {
                ASSERT(refCount < NELEM(refs));
                refs[attachmentCount].attachment = attachmentCount;
                refs[attachmentCount].layout = src->layout;
            }
            ++attachmentCount;
        }
    }

    ASSERT(attachmentCount <= NELEM(attachments));
    ASSERT(attachmentCount <= NELEM(refs));
    i32 colorRefCount = zeroIsDepth ? attachmentCount - 1 : attachmentCount;
    i32 depthRefCount = zeroIsDepth ? 1 : 0;
    const VkSubpassDescription subpass =
    {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = colorRefCount,
        .pColorAttachments = colorRefCount ? &refs[depthRefCount] : NULL,
        .pDepthStencilAttachment = depthRefCount ? &refs[0] : NULL,
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

    return vkrRenderPass_GetFull(
        attachmentCount, attachments,
        1, &subpass,
        1, &dependency);
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
