#include "rendering/vulkan/vkr_textable.h"
#include "rendering/vulkan/vkr_desc.h"
#include "rendering/vulkan/vkr_texture.h"
#include "rendering/vulkan/vkr_image.h"
#include "rendering/vulkan/vkr_sampler.h"

#include "containers/id_alloc.hpp"
#include "containers/array.hpp"
#include "common/profiler.hpp"

static constexpr vkrTextureId NullId(VkImageViewType type)
{
    vkrTextureId id = {};
    id.type = type;
    return id;
}

static GenId ToGenId(vkrTextureId id)
{
    GenId gid = {};
    gid.index = id.index;
    gid.version = id.version;
    return gid;
}

static vkrTextureId ToTexId(GenId gid, VkImageViewType type)
{
    vkrTextureId id = {};
    id.version = gid.version;
    id.type = type;
    id.index = gid.index;
    return id;
}

template<VkImageViewType t_viewType, i32 t_capacity>
class vkrTexTable
{
    IdAllocator m_ids;
    vkrImage m_images[t_capacity];
    VkImageView m_views[t_capacity];
    VkDescriptorImageInfo m_descriptors[t_capacity];

public:
    void Init()
    {
        i32 layers = 1;
        switch (t_viewType)
        {
        default:
            layers = 1;
            break;
        case VK_IMAGE_VIEW_TYPE_CUBE:
        case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
            layers = 6;
            break;
        }

        vkrTextureId id = Alloc(VK_FORMAT_R8G8B8A8_UNORM, 1, 1, 1, layers, false);
        ASSERT(id.index == 0);
        const u32 blackTexel = 0x0;
        for (i32 layer = 0; layer < layers; ++layer)
        {
            Upload(id, layer, &blackTexel, sizeof(blackTexel));
        }
        for (i32 i = 1; i < NELEM(m_descriptors); ++i)
        {
            m_descriptors[i] = m_descriptors[0];
        }
    }
    void Reset()
    {
        for (i32 index : m_ids)
        {
            vkrImageView_Del(m_views[index]);
            vkrImage_Del(&m_images[index]);
        }
        m_ids.Reset();
        memset(m_images, 0, sizeof(m_images));
        memset(m_views, 0, sizeof(m_views));
        memset(m_descriptors, 0, sizeof(m_descriptors));
    }
    void Update()
    {

    }
    void Write(VkDescriptorSet set, i32 binding)
    {
        ASSERT(set);
        ASSERT(binding >= 0);
        vkrDesc_WriteImageTable(
            set, binding,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            NELEM(m_descriptors),
            &m_descriptors[0]);
    }
    bool Exists(vkrTextureId id) const
    {
        ASSERT(id.type == t_viewType);
        return m_ids.Exists(ToGenId(id));
    }
    vkrTextureId Alloc(
        VkFormat format,
        i32 width,
        i32 height,
        i32 depth,
        i32 layers,
        bool mips)
    {
        GenId gid = m_ids.Alloc();
        i32 slot = gid.index;
        ASSERT(slot < NELEM(m_images));

        vkrImage& image = m_images[slot];
        if (!vkrTexture_New(
            &image, t_viewType, format, width, height, depth, layers, mips))
        {
            ASSERT(false);
            Free(ToTexId(gid, t_viewType));
            return NullId(t_viewType);
        }

        i32 mipCount = mips ? vkrTexture_MipCount(width, height, depth) : 1;
        VkImageView view = vkrImageView_New(
            image.handle,
            t_viewType,
            format,
            VK_IMAGE_ASPECT_COLOR_BIT,
            0, mipCount,
            0, layers);
        m_views[slot] = view;
        if (!view)
        {
            ASSERT(false);
            Free(ToTexId(gid, t_viewType));
            return NullId(t_viewType);
        }

        VkDescriptorImageInfo& desc = m_descriptors[slot];
        desc.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        desc.imageView = view;
        desc.sampler = vkrSampler_Get(
            VK_FILTER_LINEAR,
            VK_SAMPLER_MIPMAP_MODE_LINEAR,
            VK_SAMPLER_ADDRESS_MODE_REPEAT,
            4.0f);

        return ToTexId(gid, t_viewType);
    }
    bool Free(vkrTextureId id)
    {
        ASSERT(id.type == t_viewType);
        if (m_ids.Free(ToGenId(id)))
        {
            i32 slot = id.index;
            vkrImageView_Release(m_views[slot]);
            vkrImage_Release(&m_images[slot]);
            m_descriptors[slot] = m_descriptors[0];
            return true;
        }
        return false;
    }
    VkFence Upload(
        vkrTextureId id,
        i32 layer,
        void const *const data,
        i32 bytes)
    {
        ASSERT(id.type == t_viewType);
        if (Exists(id))
        {
            return vkrTexture_Upload(
                &m_images[id.index], layer, data, bytes);
        }
        return NULL;
    }
    bool SetSampler(
        vkrTextureId id,
        VkFilter filter,
        VkSamplerMipmapMode mipMode,
        VkSamplerAddressMode addressMode,
        float aniso)
    {
        ASSERT(id.type == t_viewType);
        if (Exists(id))
        {
            VkSampler sampler = vkrSampler_Get(filter, mipMode, addressMode, aniso);
            if (sampler)
            {
                m_descriptors[id.index].sampler = sampler;
                return true;
            }
        }
        return false;
    }
};

static vkrTexTable<VK_IMAGE_VIEW_TYPE_1D, kTextureTable1DSize> ms_table1D;
static vkrTexTable<VK_IMAGE_VIEW_TYPE_2D, kTextureTable2DSize> ms_table2D;
static vkrTexTable<VK_IMAGE_VIEW_TYPE_3D, kTextureTable3DSize> ms_table3D;
static vkrTexTable<VK_IMAGE_VIEW_TYPE_CUBE, kTextureTableCubeSize> ms_tableCube;
static vkrTexTable<VK_IMAGE_VIEW_TYPE_2D_ARRAY, kTextureTable2DArraySize> ms_table2DArray;

PIM_C_BEGIN

bool vkrTexTable_Init(void)
{
    ms_table1D.Init();
    ms_table2D.Init();
    ms_table3D.Init();
    ms_table2DArray.Init();
    ms_tableCube.Init();
    return true;
}

void vkrTexTable_Shutdown(void)
{
    ms_table1D.Reset();
    ms_table2D.Reset();
    ms_table3D.Reset();
    ms_table2DArray.Reset();
    ms_tableCube.Reset();
}

ProfileMark(pm_update, vkrTexTable_Update)
void vkrTexTable_Update(void)
{
    ProfileScope(pm_update);

    ms_table1D.Update();
    ms_table2D.Update();
    ms_table3D.Update();
    ms_table2DArray.Update();
    ms_tableCube.Update();
}

ProfileMark(pm_write, vkrTexTable_Write)
void vkrTexTable_Write(VkDescriptorSet set)
{
    ProfileScope(pm_write);

    ms_table1D.Write(set, vkrBindTableId_Texture1D);
    ms_table2D.Write(set, vkrBindTableId_Texture2D);
    ms_table3D.Write(set, vkrBindTableId_Texture3D);
    ms_tableCube.Write(set, vkrBindTableId_TextureCube);
    ms_table2DArray.Write(set, vkrBindTableId_Texture2DArray);
}

bool vkrTexTable_Exists(vkrTextureId id)
{
    switch (id.type)
    {
    default:
        ASSERT(false);
        return false;
    case VK_IMAGE_VIEW_TYPE_1D:
        return ms_table1D.Exists(id);
    case VK_IMAGE_VIEW_TYPE_2D:
        return ms_table2D.Exists(id);
    case VK_IMAGE_VIEW_TYPE_3D:
        return ms_table3D.Exists(id);
    case VK_IMAGE_VIEW_TYPE_CUBE:
        return ms_tableCube.Exists(id);
    case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
        return ms_table2DArray.Exists(id);
    }
}

vkrTextureId vkrTexTable_Alloc(
    VkImageViewType type,
    VkFormat format,
    i32 width,
    i32 height,
    i32 depth,
    i32 layers,
    bool mips)
{
    switch (type)
    {
    default:
        ASSERT(false);
        return NullId(VK_IMAGE_VIEW_TYPE_1D);
    case VK_IMAGE_VIEW_TYPE_1D:
    {
        ASSERT(width > 0);
        height = 1;
        depth = 1;
        layers = 1;
        return ms_table1D.Alloc(format, width, height, depth, layers, mips);
    }
    case VK_IMAGE_VIEW_TYPE_2D:
    {
        ASSERT(width > 0);
        ASSERT(height > 0);
        depth = 1;
        layers = 1;
        return ms_table2D.Alloc(format, width, height, depth, layers, mips);
    }
    case VK_IMAGE_VIEW_TYPE_3D:
    {
        ASSERT(width > 0);
        ASSERT(height > 0);
        ASSERT(depth > 0);
        layers = 1;
        return ms_table3D.Alloc(format, width, height, depth, layers, mips);
    }
    case VK_IMAGE_VIEW_TYPE_CUBE:
    {
        ASSERT(width > 0);
        ASSERT(height > 0);
        depth = 1;
        ASSERT(layers > 0);
        ASSERT((layers % 6) == 0);
        return ms_tableCube.Alloc(format, width, height, depth, layers, mips);
    }
    case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
    {
        ASSERT(width > 0);
        ASSERT(height > 0);
        depth = 1;
        ASSERT(layers > 0);
        return ms_table2DArray.Alloc(format, width, height, depth, layers, mips);
    }
    }
}

bool vkrTexTable_Free(vkrTextureId id)
{
    switch (id.type)
    {
    default:
        ASSERT(false);
        return false;
    case VK_IMAGE_VIEW_TYPE_1D:
        return ms_table1D.Free(id);
    case VK_IMAGE_VIEW_TYPE_2D:
        return ms_table2D.Free(id);
    case VK_IMAGE_VIEW_TYPE_3D:
        return ms_table3D.Free(id);
    case VK_IMAGE_VIEW_TYPE_CUBE:
        return ms_tableCube.Free(id);
    case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
        return ms_table2DArray.Free(id);
    }
}

VkFence vkrTexTable_Upload(
    vkrTextureId id,
    i32 layer,
    void const *const data,
    i32 bytes)
{
    switch (id.type)
    {
    default:
        ASSERT(false);
        return NULL;
    case VK_IMAGE_VIEW_TYPE_1D:
    {
        layer = 0;
        return ms_table1D.Upload(id, layer, data, bytes);
    }
    case VK_IMAGE_VIEW_TYPE_2D:
    {
        layer = 0;
        return ms_table2D.Upload(id, layer, data, bytes);
    }
    case VK_IMAGE_VIEW_TYPE_3D:
    {
        layer = 0;
        return ms_table3D.Upload(id, layer, data, bytes);
    }
    case VK_IMAGE_VIEW_TYPE_CUBE:
    {
        ASSERT(layer >= 0);
        ASSERT(layer < 6);
        return ms_tableCube.Upload(id, layer, data, bytes);
    }
    case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
    {
        ASSERT(layer >= 0);
        return ms_table2DArray.Upload(id, layer, data, bytes);
    }
    }
}

bool vkrTexTable_SetSampler(
    vkrTextureId id,
    VkFilter filter,
    VkSamplerMipmapMode mipMode,
    VkSamplerAddressMode addressMode,
    float aniso)
{
    switch (id.type)
    {
    default:
        ASSERT(false);
        return false;
    case VK_IMAGE_VIEW_TYPE_1D:
        return ms_table1D.SetSampler(id, filter, mipMode, addressMode, aniso);
    case VK_IMAGE_VIEW_TYPE_2D:
        return ms_table2D.SetSampler(id, filter, mipMode, addressMode, aniso);
    case VK_IMAGE_VIEW_TYPE_3D:
        return ms_table3D.SetSampler(id, filter, mipMode, addressMode, aniso);
    case VK_IMAGE_VIEW_TYPE_CUBE:
        return ms_tableCube.SetSampler(id, filter, mipMode, addressMode, aniso);
    case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
        return ms_table2DArray.SetSampler(id, filter, mipMode, addressMode, aniso);
    }
}

PIM_C_END
