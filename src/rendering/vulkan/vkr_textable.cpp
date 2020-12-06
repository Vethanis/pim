#include "rendering/vulkan/vkr_textable.h"
#include "rendering/vulkan/vkr_desc.h"
#include "rendering/vulkan/vkr_texture.h"

#include "containers/id_alloc.hpp"
#include "containers/array.hpp"
#include "common/profiler.hpp"

pim_inline GenId ToGenId(vkrTextureId id)
{
    GenId gid;
    gid.asint = id.asint;
    return gid;
}

pim_inline vkrTextureId ToTexId(GenId gid)
{
    vkrTextureId id;
    id.asint = gid.asint;
    return id;
}

class vkrTexTable
{
    IdAllocator m_ids;
    vkrTexture2D m_textures[kTextureDescriptors];
    VkDescriptorImageInfo m_descriptors[kTextureDescriptors];

public:
    void Init()
    {
        GenId id = m_ids.Alloc();
        ASSERT(id.index == 0);
        vkrTexture2D& black = m_textures[0];
        const u32 blackTexel = 0x0;
        vkrTexture2D_New(&black, 1, 1, VK_FORMAT_R8G8B8A8_SRGB, &blackTexel, sizeof(blackTexel));
        VkDescriptorImageInfo& desc = m_descriptors[0];
        desc.imageLayout = black.image.layout;
        desc.imageView = black.view;
        desc.sampler = black.sampler;
    }
    void Reset()
    {
        for (vkrTexture2D& texture : m_textures)
        {
            vkrTexture2D_Del(&texture);
        }
        m_ids.Reset();
        memset(m_textures, 0, sizeof(m_textures));
        memset(m_descriptors, 0, sizeof(m_descriptors));
    }
    void Update()
    {
        vkrTexture2D const *const textures = &m_textures[0];
        VkDescriptorImageInfo *const descriptors = &m_descriptors[0];
        for (i32 i = 0; i < kTextureDescriptors; ++i)
        {
            if (textures[i].view)
            {
                descriptors[i].imageLayout = textures[i].image.layout;
                descriptors[i].imageView = textures[i].view;
                descriptors[i].sampler = textures[i].sampler;
            }
            else
            {
                descriptors[i].imageLayout = textures[0].image.layout;
                descriptors[i].imageView = textures[0].view;
                descriptors[i].sampler = textures[0].sampler;
            }
        }
    }
    void Write(VkDescriptorSet set, i32 binding)
    {
        ASSERT(set);
        ASSERT(binding >= 0);
        vkrDesc_WriteImageTable(
            set, binding,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            kTextureDescriptors,
            &m_descriptors[0]);
    }
    bool Exists(vkrTextureId id) const
    {
        return m_ids.Exists(ToGenId(id));
    }
    vkrTextureId Alloc(i32 width, i32 height, VkFormat format, void const *const data)
    {
        GenId gid = m_ids.Alloc();
        i32 slot = gid.index;

        vkrTexture2D& tex = m_textures[slot];
        i32 bpp = vkrFormatToBpp(format);
        i32 bytes = (width * height * bpp) / 8;
        if (!vkrTexture2D_New(&tex, width, height, format, data, data ? bytes : 0))
        {
            ASSERT(false);
            m_ids.Free(gid);
            vkrTextureId id = {};
            return id;
        }

        VkDescriptorImageInfo& desc = m_descriptors[slot];
        desc.imageLayout = tex.image.layout;
        desc.imageView = tex.view;
        desc.sampler = tex.sampler;

        return ToTexId(gid);
    }
    bool Free(vkrTextureId id)
    {
        GenId gid = ToGenId(id);
        if (m_ids.Free(ToGenId(id)))
        {
            i32 slot = gid.index;
            vkrTexture2D_Release(&m_textures[slot]);
            VkDescriptorImageInfo& desc = m_descriptors[slot];
            const vkrTexture2D& black = m_textures[0];
            desc.imageLayout = black.image.layout;
            desc.imageView = black.view;
            desc.sampler = black.sampler;
            return true;
        }
        return false;
    }
    VkFence Upload(vkrTextureId id, void const *const data, i32 bytes)
    {
        if (Exists(id))
        {
            return vkrTexture2D_Upload(&m_textures[id.index], data, bytes);
        }
        return NULL;
    }
};

static vkrTexTable ms_table;

bool vkrTexTable_Init(void)
{
    ms_table.Init();
    return true;
}

void vkrTexTable_Shutdown(void)
{
    ms_table.Reset();
}

ProfileMark(pm_update, vkrTexTable_Update)
void vkrTexTable_Update(void)
{
    ProfileScope(pm_update);

    ms_table.Update();
}

ProfileMark(pm_write, vkrTexTable_Write)
void vkrTexTable_Write(VkDescriptorSet set, i32 binding)
{
    ProfileScope(pm_write);

    ms_table.Write(set, binding);
}

bool vkrTexTable_Exists(vkrTextureId id)
{
    return ms_table.Exists(id);
}

vkrTextureId vkrTexTable_Alloc(
    i32 width,
    i32 height,
    VkFormat format,
    void const *const data)
{
    return ms_table.Alloc(width, height, format, data);
}

bool vkrTexTable_Free(vkrTextureId id)
{
    return ms_table.Free(id);
}

VkFence vkrTexTable_Upload(vkrTextureId id, void const *const data, i32 bytes)
{
    return ms_table.Upload(id, data, bytes);
}
