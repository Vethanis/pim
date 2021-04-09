#include "rendering/vulkan/vkr_textable.h"
#include "rendering/vulkan/vkr_desc.h"
#include "rendering/vulkan/vkr_texture.h"
#include "rendering/vulkan/vkr_image.h"
#include "rendering/vulkan/vkr_sampler.h"

#include "allocator/allocator.h"
#include "containers/idalloc.h"
#include "common/profiler.h"

#include <string.h>

// ----------------------------------------------------------------------------

static vkrTextureId NullId(VkImageViewType type)
{
    vkrTextureId id = { 0 };
    id.type = type;
    return id;
}

static GenId ToGenId(vkrTextureId id)
{
    GenId gid = { 0 };
    gid.index = id.index;
    gid.version = id.version;
    return gid;
}

static vkrTextureId ToTexId(GenId gid, VkImageViewType type)
{
    vkrTextureId id = { 0 };
    id.version = gid.version;
    id.type = type;
    id.index = gid.index;
    return id;
}

// ----------------------------------------------------------------------------

typedef struct TexTable_s
{
    IdAlloc ids;
    vkrImage* images;
    VkImageView* views;
    VkDescriptorImageInfo* descriptors;
    i32 capacity;
    VkImageViewType viewType;
} TexTable;

static void TexTable_New(TexTable* tt, VkImageViewType viewType, i32 capacity);
static void TexTable_Del(TexTable* tt);
static void TexTable_Write(TexTable* tt, VkDescriptorSet set, i32 binding);
static bool TexTable_Exists(const TexTable* tt, vkrTextureId id);
static vkrTextureId TexTable_Alloc(
    TexTable* tt,
    VkFormat format,
    i32 width,
    i32 height,
    i32 depth,
    i32 layers,
    bool mips);
static bool TexTable_Free(TexTable* tt, vkrTextureId id);
static VkFence TexTable_Upload(
    TexTable* tt,
    vkrTextureId id,
    i32 layer,
    void const *const data,
    i32 bytes);
static bool TexTable_SetSampler(
    TexTable* tt,
    vkrTextureId id,
    VkFilter filter,
    VkSamplerMipmapMode mipMode,
    VkSamplerAddressMode addressMode,
    float aniso);

// ----------------------------------------------------------------------------

static void TexTable_New(TexTable* tt, VkImageViewType viewType, i32 capacity)
{
    ASSERT(capacity > 0);
    memset(tt, 0, sizeof(*tt));

    IdAlloc_New(&tt->ids);
    tt->images = Perm_Calloc(sizeof(tt->images[0]) * capacity);
    tt->views = Perm_Calloc(sizeof(tt->views[0]) * capacity);
    tt->descriptors = Perm_Calloc(sizeof(tt->descriptors[0]) * capacity);
    tt->capacity = capacity;
    tt->viewType = viewType;

    i32 layers = 1;
    switch (viewType)
    {
    default:
        layers = 1;
        break;
    case VK_IMAGE_VIEW_TYPE_CUBE:
        layers = 6;
        break;
    case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
        layers = 255;
        break;
    }

    vkrTextureId id = TexTable_Alloc(tt, VK_FORMAT_R8G8B8A8_UNORM, 1, 1, 1, layers, false);
    ASSERT(id.index == 0);
    const u32 blackTexel = 0x0;
    for (i32 layer = 0; layer < layers; ++layer)
    {
        TexTable_Upload(tt, id, layer, &blackTexel, sizeof(blackTexel));
    }
    for (i32 i = 1; i < capacity; ++i)
    {
        tt->descriptors[i] = tt->descriptors[0];
    }
}

static void TexTable_Del(TexTable* tt)
{
    const i32 len = IdAlloc_Capacity(&tt->ids);
    for (i32 i = 0; i < len; ++i)
    {
        if (IdAlloc_ExistsAt(&tt->ids, i))
        {
            vkrImageView_Release(tt->views[i]);
            vkrImage_Release(&tt->images[i]);
        }
    }
    IdAlloc_Del(&tt->ids);
    Mem_Free(tt->images);
    Mem_Free(tt->views);
    Mem_Free(tt->descriptors);
    memset(tt, 0, sizeof(*tt));
}

static void TexTable_Write(TexTable* tt, VkDescriptorSet set, i32 binding)
{
    ASSERT(set);
    ASSERT(binding >= 0);
    vkrDesc_WriteImageTable(
        set, binding,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        tt->capacity,
        tt->descriptors);
}

static bool TexTable_Exists(const TexTable* tt, vkrTextureId id)
{
    ASSERT(id.type == tt->viewType);
    return IdAlloc_Exists(&tt->ids, ToGenId(id));
}

static vkrTextureId TexTable_Alloc(
    TexTable* tt,
    VkFormat format,
    i32 width,
    i32 height,
    i32 depth,
    i32 layers,
    bool mips)
{
    const VkImageViewType viewType = tt->viewType;
    const GenId gid = IdAlloc_Alloc(&tt->ids);
    const i32 slot = gid.index;
    ASSERT(slot < tt->capacity);
    ASSERT(tt->capacity > 0);

    vkrImage* image = &tt->images[slot];
    if (!vkrTexture_New(image, viewType, format, width, height, depth, layers, mips))
    {
        ASSERT(false);
        TexTable_Free(tt, ToTexId(gid, viewType));
        return NullId(viewType);
    }

    const i32 mipCount = mips ? vkrTexture_MipCount(width, height, depth) : 1;
    VkImageView view = vkrImageView_New(
        image->handle,
        viewType,
        format,
        VK_IMAGE_ASPECT_COLOR_BIT,
        0, mipCount,
        0, layers);
    tt->views[slot] = view;
    if (!view)
    {
        ASSERT(false);
        TexTable_Free(tt, ToTexId(gid, viewType));
        return NullId(viewType);
    }

    VkDescriptorImageInfo* desc = &tt->descriptors[slot];
    desc->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    desc->imageView = view;
    desc->sampler = vkrSampler_Get(
        VK_FILTER_LINEAR,
        VK_SAMPLER_MIPMAP_MODE_LINEAR,
        VK_SAMPLER_ADDRESS_MODE_REPEAT,
        4.0f);

    return ToTexId(gid, viewType);
}

static bool TexTable_Free(TexTable* tt, vkrTextureId id)
{
    ASSERT(id.type == tt->viewType);
    if (IdAlloc_Free(&tt->ids, ToGenId(id)))
    {
        i32 slot = id.index;
        ASSERT(slot >= 0);
        ASSERT(slot < tt->capacity);
        vkrImageView_Release(tt->views[slot]);
        vkrImage_Release(&tt->images[slot]);
        tt->descriptors[slot] = tt->descriptors[0];
        return true;
    }
    return false;
}

static VkFence TexTable_Upload(
    TexTable* tt,
    vkrTextureId id,
    i32 layer,
    void const *const data,
    i32 bytes)
{
    if (TexTable_Exists(tt, id))
    {
        return vkrTexture_Upload(&tt->images[id.index], layer, data, bytes);
    }
    return NULL;
}

static bool TexTable_SetSampler(
    TexTable* tt,
    vkrTextureId id,
    VkFilter filter,
    VkSamplerMipmapMode mipMode,
    VkSamplerAddressMode addressMode,
    float aniso)
{
    ASSERT(id.type == tt->viewType);
    if (TexTable_Exists(tt, id))
    {
        VkSampler sampler = vkrSampler_Get(filter, mipMode, addressMode, aniso);
        if (sampler)
        {
            tt->descriptors[id.index].sampler = sampler;
            return true;
        }
    }
    return false;
}

// ----------------------------------------------------------------------------

typedef enum
{
    TexTableType_1D,
    TexTableType_2D,
    TexTableType_3D,
    TexTableType_Cube,
    TexTableType_2DArray,

    TexTableType_COUNT
} TexTableType;

static const VkImageViewType kTableViewTypes[] =
{
    VK_IMAGE_VIEW_TYPE_1D,
    VK_IMAGE_VIEW_TYPE_2D,
    VK_IMAGE_VIEW_TYPE_3D,
    VK_IMAGE_VIEW_TYPE_CUBE,
    VK_IMAGE_VIEW_TYPE_2D_ARRAY,
};
SASSERT(NELEM(kTableViewTypes) == TexTableType_COUNT);

static const i32 kTableCapacities[] =
{
    kTextureTable1DSize,
    kTextureTable2DSize,
    kTextureTable3DSize,
    kTextureTableCubeSize,
    kTextureTable2DArraySize,
};
SASSERT(NELEM(kTableCapacities) == TexTableType_COUNT);

static const i32 kTableBindIds[] =
{
    vkrBindTableId_Texture1D,
    vkrBindTableId_Texture2D,
    vkrBindTableId_Texture3D,
    vkrBindTableId_TextureCube,
    vkrBindTableId_Texture2DArray,
};
SASSERT(NELEM(kTableBindIds) == TexTableType_COUNT);

static TexTable ms_tables[TexTableType_COUNT];

static TexTable* GetTexTable(VkImageViewType viewType)
{
    switch (viewType)
    {
    default:
        ASSERT(false);
        return NULL;
    case VK_IMAGE_VIEW_TYPE_1D:
        return &ms_tables[TexTableType_1D];
    case VK_IMAGE_VIEW_TYPE_2D:
        return &ms_tables[TexTableType_2D];
    case VK_IMAGE_VIEW_TYPE_3D:
        return &ms_tables[TexTableType_3D];
    case VK_IMAGE_VIEW_TYPE_CUBE:
        return &ms_tables[TexTableType_Cube];
    case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
        return &ms_tables[TexTableType_2DArray];
    }
}

// ----------------------------------------------------------------------------

bool vkrTexTable_Init(void)
{
    for (i32 i = 0; i < NELEM(ms_tables); ++i)
    {
        TexTable_New(&ms_tables[i], kTableViewTypes[i], kTableCapacities[i]);
    }
    return true;
}

void vkrTexTable_Shutdown(void)
{
    for (i32 i = 0; i < NELEM(ms_tables); ++i)
    {
        TexTable_Del(&ms_tables[i]);
    }
}

void vkrTexTable_Update(void)
{

}

ProfileMark(pm_write, vkrTexTable_Write)
void vkrTexTable_Write(VkDescriptorSet set)
{
    ProfileBegin(pm_write);

    for (i32 i = 0; i < NELEM(ms_tables); ++i)
    {
        TexTable_Write(&ms_tables[i], set, kTableBindIds[i]);
    }

    ProfileEnd(pm_write);
}

bool vkrTexTable_Exists(vkrTextureId id)
{
    return TexTable_Exists(GetTexTable(id.type), id);
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
        ASSERT(width > 0);
        height = 1;
        depth = 1;
        layers = 1;
        break;
    case VK_IMAGE_VIEW_TYPE_2D:
        ASSERT(width > 0);
        ASSERT(height > 0);
        depth = 1;
        layers = 1;
        break;
    case VK_IMAGE_VIEW_TYPE_3D:
        ASSERT(width > 0);
        ASSERT(height > 0);
        ASSERT(depth > 0);
        layers = 1;
        break;
    case VK_IMAGE_VIEW_TYPE_CUBE:
        ASSERT(width > 0);
        ASSERT(height > 0);
        depth = 1;
        layers = 6;
        break;
    case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
        ASSERT(width > 0);
        ASSERT(height > 0);
        depth = 1;
        ASSERT(layers > 0);
        ASSERT(layers <= 255);
        break;
    }
    return TexTable_Alloc(GetTexTable(type), format, width, height, depth, layers, mips);
}

bool vkrTexTable_Free(vkrTextureId id)
{
    return TexTable_Free(GetTexTable(id.type), id);
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
        layer = 0;
        break;
    case VK_IMAGE_VIEW_TYPE_2D:
        layer = 0;
        break;
    case VK_IMAGE_VIEW_TYPE_3D:
        layer = 0;
        break;
    case VK_IMAGE_VIEW_TYPE_CUBE:
        ASSERT(layer >= 0);
        ASSERT(layer < 6);
        break;
    case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
        ASSERT(layer >= 0);
        ASSERT(layer < 256);
        break;
    }
    return TexTable_Upload(GetTexTable(id.type), id, layer, data, bytes);
}

bool vkrTexTable_SetSampler(
    vkrTextureId id,
    VkFilter filter,
    VkSamplerMipmapMode mipMode,
    VkSamplerAddressMode addressMode,
    float aniso)
{
    return TexTable_SetSampler(GetTexTable(id.type), id, filter, mipMode, addressMode, aniso);
}

