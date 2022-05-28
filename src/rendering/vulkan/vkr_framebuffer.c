#include "rendering/vulkan/vkr_framebuffer.h"
#include "rendering/vulkan/vkr_renderpass.h"

#include "allocator/allocator.h"
#include "containers/dict.h"
#include <string.h>

// ----------------------------------------------------------------------------

typedef struct Key_s
{
    VkImageView attachments[8];
    VkFormat formats[8];
    u32 width : 16;
    u32 height : 16;
} Key;

typedef struct Value_s
{
    VkFramebuffer handle;
} Value;

static VkFramebuffer vkrFramebuffer_New(
    const VkImageView* attachments,
    const VkFormat* formats,
    i32 attachmentCount,
    i32 width,
    i32 height);
static bool ContainsView(const Key* key, VkImageView view);

// ----------------------------------------------------------------------------

static Dict ms_fbufs;

// ----------------------------------------------------------------------------

bool vkrFramebuffer_Init(void)
{
    Dict_New(&ms_fbufs, sizeof(Key), sizeof(Value), EAlloc_Perm);
    Dict_Reserve(&ms_fbufs, 16);
    return true;
}

void vkrFramebuffer_Shutdown(void)
{
    ASSERT(g_vkr.dev);
    u32 width = ms_fbufs.width;
    Value* values = ms_fbufs.values;
    for (u32 i = 0; i < width; ++i)
    {
        if (values[i].handle)
        {
            vkDestroyFramebuffer(g_vkr.dev, values[i].handle, NULL);
            values[i].handle = NULL;
        }
    }
    Dict_Del(&ms_fbufs);
}

VkFramebuffer vkrFramebuffer_Get(
    const vkrImage** attachments,
    i32 count,
    i32 width,
    i32 height)
{
    ASSERT(count >= 1);
    ASSERT(count <= 8);
    Value value = { 0 };
    Key key = { 0 };
    for (i32 i = 0; i < count; ++i)
    {
        if (attachments[i] && attachments[i]->view)
        {
            key.attachments[i] = attachments[i]->view;
            key.formats[i] = attachments[i]->format;
        }
    }
    key.width = (u32)width;
    key.height = (u32)height;
    if (!Dict_Get(&ms_fbufs, &key, &value))
    {
        value.handle = vkrFramebuffer_New(key.attachments, key.formats, count, key.width, key.height);
        if (value.handle)
        {
            if (!Dict_Add(&ms_fbufs, &key, &value))
            {
                ASSERT(false);
            }
        }
    }
    return value.handle;
}

void vkrFramebuffer_Remove(VkImageView view)
{
    if (!view)
    {
        return;
    }

    u32 width = Dict_GetWidth(&ms_fbufs);
    const Key* keys = ms_fbufs.keys;
    Value* values = ms_fbufs.values;
    for (u32 i = 0; i < width; ++i)
    {
        if (values[i].handle && ContainsView(&keys[i], view))
        {
            vkDestroyFramebuffer(g_vkr.dev, values[i].handle, NULL);
            Dict_RmAt(&ms_fbufs, i, NULL);
        }
    }
}

// ----------------------------------------------------------------------------

static VkFramebuffer vkrFramebuffer_New(
    const VkImageView* attachments,
    const VkFormat* formats,
    i32 attachmentCount,
    i32 width,
    i32 height)
{
    VkFramebuffer handle = NULL;

    // make up a compatible render pass
    vkrRenderPassDesc passDesc =
    {
        .srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    };
    for (i32 i = 0; i < attachmentCount; ++i)
    {
        passDesc.attachments[i].format = formats[i];
        VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        if ((i == 0) &&
            (formats[i] >= VK_FORMAT_D16_UNORM) &&
            (formats[i] <= VK_FORMAT_D32_SFLOAT_S8_UINT))
        {
            layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }
        passDesc.attachments[i].initialLayout = layout;
        passDesc.attachments[i].layout = layout;
        passDesc.attachments[i].finalLayout = layout;
        //passDesc.attachments[i].load = VK_ATTACHMENT_LOAD_OP_LOAD;
        //passDesc.attachments[i].store = VK_ATTACHMENT_STORE_OP_STORE;
    }
    VkRenderPass pass = vkrRenderPass_Get(&passDesc);
    ASSERT(pass);
    if (pass)
    {
        const VkFramebufferCreateInfo bufferInfo =
        {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = pass,
            .attachmentCount = attachmentCount,
            .pAttachments = attachments,
            .width = width,
            .height = height,
            .layers = 1,
        };
        VkCheck(vkCreateFramebuffer(g_vkr.dev, &bufferInfo, NULL, &handle));
        ASSERT(handle);
    }

    return handle;
}

static bool ContainsView(const Key* key, VkImageView view)
{
    for (u32 i = 0; i < NELEM(key->attachments); ++i)
    {
        if (key->attachments[i] == view)
        {
            return true;
        }
    }
    return false;
}
