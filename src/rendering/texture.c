#include "rendering/texture.h"
#include "allocator/allocator.h"
#include "containers/table.h"
#include "math/color.h"
#include "math/blending.h"
#include "math/bool_funcs.h"
#include "rendering/sampler.h"
#include "rendering/material.h"
#include "assets/asset_system.h"
#include "stb/stb_image.h"
#include "common/stringutil.h"
#include "ui/cimgui_ext.h"
#include "common/sort.h"
#include "common/profiler.h"
#include "io/fstr.h"
#include "threading/task.h"
#include "rendering/vulkan/vkr_texture.h"
#include "rendering/vulkan/vkr_textable.h"
#include "assets/crate.h"
#include "common/nextpow2.h"
#include <string.h>

static Table ms_table;

static void ResizeToPow2(Texture* tex);

static GenId ToGenId(TextureId tid)
{
    GenId gid;
    gid.index = tid.index;
    gid.version = tid.version;
    return gid;
}

static TextureId ToTexId(GenId gid)
{
    TextureId tid;
    tid.index = gid.index;
    tid.version = gid.version;
    return tid;
}

static bool IsCurrent(TextureId id)
{
    return Table_Exists(&ms_table, ToGenId(id));
}

static void FreeTexture(Texture* tex)
{
    Mem_Free(tex->texels);
    vkrTexTable_Free(tex->slot);
    memset(tex, 0, sizeof(*tex));
}

Table const *const Texture_GetTable(void)
{
    return &ms_table;
}

void TextureSys_Init(void)
{
    Table_New(&ms_table, sizeof(Texture));

}

void TextureSys_Update(void)
{

}

void TextureSys_Shutdown(void)
{
    Texture *const pim_noalias textures = ms_table.values;
    const i32 width = ms_table.width;
    for (i32 i = 0; i < width; ++i)
    {
        if (textures[i].texels)
        {
            FreeTexture(&textures[i]);
        }
    }
    Table_Del(&ms_table);
}

ProfileMark(pm_loadat, Texture_LoadAt)
bool Texture_LoadAt(const char* path, VkFormat format, VkSamplerAddressMode clamp, TextureId* idOut)
{
    ProfileBegin(pm_loadat);

    bool loaded = false;
    i32 width = 0;
    i32 height = 0;
    i32 channels = 0;
    // STBI_MALLOC => Tex_Alloc(bytes)
    u32* pim_noalias texels = (u32*)stbi_load(path, &width, &height, &channels, 4);
    if (texels)
    {
        Texture tex = { 0 };
        tex.size = (int2) { width, height };
        tex.texels = texels;
        Guid name = Guid_FromStr(path);
        loaded = Texture_New(&tex, format, clamp, name, idOut);
    }

    ProfileEnd(pm_loadat);
    return loaded;
}

ProfileMark(pm_new, Texture_New)
bool Texture_New(
    Texture* tex,
    VkFormat format,
    VkSamplerAddressMode clamp,
    Guid name,
    TextureId* idOut)
{
    ProfileBegin(pm_new);
    ASSERT(tex);
    ASSERT(idOut);
    ASSERT(tex->size.x > 0);
    ASSERT(tex->size.y > 0);
    ASSERT(tex->texels);

    bool added = false;
    GenId id = { 0 };
    if (tex->texels)
    {
        tex->format = format;
        if (Table_Find(&ms_table, name, &id))
        {
            Table_Retain(&ms_table, id);
        }
        else
        {
            ResizeToPow2(tex);
            i32 width = tex->size.x;
            i32 height = tex->size.y;
            tex->slot = vkrTexTable_Alloc(
                VK_IMAGE_VIEW_TYPE_2D,
                format,
                clamp,
                width,
                height,
                1, // depth
                1, // layers
                true); // mips
            i32 bytes = (width * height * vkrFormatToBpp(format)) / 8;
            if (vkrTexTable_Upload(tex->slot, 0, tex->texels, bytes))
            {
                added = Table_Add(&ms_table, name, tex, &id);
            }
            ASSERT(added);
        }
    }
    *idOut = ToTexId(id);
    if (!added)
    {
        FreeTexture(tex);
    }
    ProfileEnd(pm_new);
    return added;
}

ProfileMark(pm_upload, Texture_Upload)
bool Texture_Upload(TextureId id)
{
    ProfileBegin(pm_upload);
    bool uploaded = false;
    Texture* tex = Texture_Get(id);
    if (tex)
    {
        ResizeToPow2(tex);
        i32 width = tex->size.x;
        i32 height = tex->size.y;
        i32 bytes = (width * height * vkrFormatToBpp(tex->format)) / 8;
        uploaded = vkrTexTable_Upload(tex->slot, 0, tex->texels, bytes);
    }
    ProfileEnd(pm_upload);
    return uploaded;
}

bool Texture_Exists(TextureId id)
{
    return IsCurrent(id);
}

void Texture_Retain(TextureId id)
{
    Table_Retain(&ms_table, ToGenId(id));
}

ProfileMark(pm_release, Texture_Release)
void Texture_Release(TextureId id)
{
    ProfileBegin(pm_release);
    Texture tex = { 0 };
    if (Table_Release(&ms_table, ToGenId(id), &tex))
    {
        FreeTexture(&tex);
    }
    ProfileEnd(pm_release);
}

Texture* Texture_Get(TextureId id)
{
    return Table_Get(&ms_table, ToGenId(id));
}

bool Texture_Find(Guid name, TextureId* idOut)
{
    ASSERT(idOut);
    GenId id;
    bool found = Table_Find(&ms_table, name, &id);
    *idOut = ToTexId(id);
    return found;
}

bool Texture_GetGuid(TextureId tid, Guid* nameOut)
{
    GenId gid = ToGenId(tid);
    return Table_GetName(&ms_table, gid, nameOut);
}

bool Texture_GetName(TextureId tid, char* dst, i32 size)
{
    dst[0] = 0;
    Guid guid;
    if (Texture_GetGuid(tid, &guid))
    {
        return Guid_GetName(guid, dst, size);
    }
    return false;
}

ProfileMark(pm_save, Texture_Save)
bool Texture_Save(Crate* crate, TextureId tid, Guid* dst)
{
    ProfileBegin(pm_save);

    bool wasSet = false;
    if (Texture_GetGuid(tid, dst))
    {
        const Texture* textures = ms_table.values;
        const Texture texture = textures[tid.index];
        const i32 len = texture.size.x * texture.size.y;
        const i32 bpp = vkrFormatToBpp(texture.format);
        const i32 texelBytes = (bpp * len) / 8;
        const i32 hdrBytes = sizeof(DiskTexture);
        DiskTexture* dtexture = Tex_Alloc(hdrBytes + texelBytes);
        if (dtexture)
        {
            dtexture->version = kTextureVersion;
            dtexture->format = texture.format;
            dtexture->size = texture.size;
            Guid_GetName(*dst, ARGS(dtexture->name));
            memcpy(dtexture + 1, texture.texels, texelBytes);
            wasSet = Crate_Set(crate, *dst, dtexture, hdrBytes + texelBytes);
        }
        Mem_Free(dtexture);
    }

    ProfileEnd(pm_save);
    return wasSet;
}

ProfileMark(pm_load, Texture_Load)
bool Texture_Load(Crate* crate, Guid name, TextureId* dst)
{
    ProfileBegin(pm_load);

    bool loaded = false;
    if (Texture_Find(name, dst))
    {
        Texture_Retain(*dst);
        loaded = true;
        goto cleanup;
    }

    Texture texture = { 0 };
    i32 offset = 0;
    i32 size = 0;
    if (Crate_Stat(crate, name, &offset, &size))
    {
        DiskTexture* dtexture = Tex_Alloc(size);
        if (dtexture && Crate_Get(crate, name, dtexture, size))
        {
            if (dtexture->version == kTextureVersion)
            {
                const VkFormat format = dtexture->format;
                const i32 bpp = vkrFormatToBpp(format);
                texture.size = dtexture->size;
                if ((texture.size.x > 0) && (texture.size.y > 0) && (bpp > 0))
                {
                    Guid_FromStr(dtexture->name);
                    const i32 len = texture.size.x * texture.size.y;
                    const i32 texelBytes = (bpp * len) / 8;
                    memmove(dtexture, dtexture + 1, texelBytes);
                    texture.texels = (u32*)dtexture;
                    dtexture = NULL;
                    loaded = Texture_New(&texture, format, VK_SAMPLER_ADDRESS_MODE_REPEAT, name, dst);
                }
            }
        }
        Mem_Free(dtexture);
    }

cleanup:
    ProfileEnd(pm_load);
    return loaded;
}

typedef struct Task_ResizeToPow2
{
    Task task;
    int2 oldSize;
    int2 newSize;
    void* pim_noalias src;
    void* pim_noalias dst;
} Task_ResizeToPow2;

static void ResizeToPow2Fn_f4(void* pbase, i32 begin, i32 end)
{
    Task_ResizeToPow2* task = pbase;
    int2 oldSize = task->oldSize;
    int2 newSize = task->newSize;
    float4* pim_noalias src = task->src;
    float4* pim_noalias dst = task->dst;
    for (i32 i = begin; i < end; ++i)
    {
        dst[i] = UvBilinearClamp_f4(src, oldSize, IndexToUv(newSize, i));
    }
}
static void ResizeToPow2Fn_c32(void* pbase, i32 begin, i32 end)
{
    Task_ResizeToPow2* task = pbase;
    int2 oldSize = task->oldSize;
    int2 newSize = task->newSize;
    R8G8B8A8_t* pim_noalias src = task->src;
    R8G8B8A8_t* pim_noalias dst = task->dst;
    for (i32 i = begin; i < end; ++i)
    {
        dst[i] = GammaEncode_rgba8(UvBilinearClamp_c32(src, oldSize, IndexToUv(newSize, i)));
    }
}
static void ResizeToPow2Fn_dir8(void* pbase, i32 begin, i32 end)
{
    Task_ResizeToPow2* task = pbase;
    int2 oldSize = task->oldSize;
    int2 newSize = task->newSize;
    R8G8B8A8_t* pim_noalias src = task->src;
    R8G8B8A8_t* pim_noalias dst = task->dst;
    for (i32 i = begin; i < end; ++i)
    {
        dst[i] = DirectionToColor(UvBilinearClamp_dir8(src, oldSize, IndexToUv(newSize, i)));
    }
}
static void ResizeToPow2Fn_xy16(void* pbase, i32 begin, i32 end)
{
    Task_ResizeToPow2* task = pbase;
    int2 oldSize = task->oldSize;
    int2 newSize = task->newSize;
    short2* pim_noalias src = task->src;
    short2* pim_noalias dst = task->dst;
    for (i32 i = begin; i < end; ++i)
    {
        dst[i] = NormalTsToXy16(UvBilinearClamp_xy16(src, oldSize, IndexToUv(newSize, i)));
    }
}

ProfileMark(pm_resizepow2, ResizeToPow2)
static void ResizeToPow2(Texture* tex)
{
    ProfileBegin(pm_resizepow2);

    const int2 oldSize = tex->size;
    const int2 newSize = {
        .x = NextPow2(oldSize.x),
        .y = NextPow2(oldSize.y),
    };
    if (b2_all(i2_eq(oldSize, newSize)))
    {
        goto cleanup;
    }

    const i32 newLen = newSize.x * newSize.y;
    Task_ResizeToPow2* task = Temp_Calloc(sizeof(*task));
    task->src = tex->texels;
    task->oldSize = oldSize;
    task->newSize = newSize;

    switch (tex->format)
    {
    default:
        ASSERT(false);
        break;
    case VK_FORMAT_R32G32B32A32_SFLOAT:
    {
        task->dst = Tex_Alloc(newLen * sizeof(float4));
        Task_Run(task, ResizeToPow2Fn_f4, newLen);
        Mem_Free(tex->texels);
        tex->texels = task->dst;
        tex->size = newSize;
    }
    break;
    case VK_FORMAT_R8G8B8A8_SRGB:
    {
        task->dst = Tex_Alloc(newLen * sizeof(R8G8B8A8_t));
        Task_Run(task, ResizeToPow2Fn_c32, newLen);
        Mem_Free(tex->texels);
        tex->texels = task->dst;
        tex->size = newSize;
    }
    break;
    case VK_FORMAT_R8G8B8A8_UNORM:
    {
        task->dst = Tex_Alloc(newLen * sizeof(R8G8B8A8_t));
        Task_Run(task, ResizeToPow2Fn_dir8, newLen);
        Mem_Free(tex->texels);
        tex->texels = task->dst;
        tex->size = newSize;
    }
    break;
    case VK_FORMAT_R16G16_SNORM:
    {
        task->dst = Tex_Alloc(newLen * sizeof(short2));
        Task_Run(task, ResizeToPow2Fn_xy16, newLen);
        Mem_Free(tex->texels);
        tex->texels = task->dst;
        tex->size = newSize;
    }
    break;
    }

cleanup:
    ProfileEnd(pm_resizepow2);
}

// ----------------------------------------------------------------------------

static bool gs_revSort;
static i32 gs_cmpMode;
static i32 gs_selection;
static char gs_search[PIM_PATH];

static i32 CmpSlotFn(i32 ilhs, i32 irhs, void* usr)
{
    const Guid* pim_noalias names = ms_table.names;
    const Texture* pim_noalias textures = ms_table.values;
    const i32* pim_noalias refcounts = ms_table.refcounts;

    Guid lhs = names[ilhs];
    Guid rhs = names[irhs];
    bool lvalid = !Guid_IsNull(lhs);
    bool rvalid = !Guid_IsNull(rhs);
    if (lvalid && rvalid)
    {
        i32 cmp = 0;
        switch (gs_cmpMode)
        {
        default:
        case 0:
        {
            char lname[64] = { 0 };
            char rname[64] = { 0 };
            Guid_GetName(lhs, ARGS(lname));
            Guid_GetName(rhs, ARGS(rname));
            cmp = StrCmp(ARGS(lname), rname);
        }
        break;
        case 1:
            cmp = textures[ilhs].size.x - textures[irhs].size.x;
            break;
        case 2:
            cmp = textures[ilhs].size.y - textures[irhs].size.y;
            break;
        case 3:
            cmp = refcounts[ilhs] - refcounts[irhs];
            break;
        }
        return gs_revSort ? -cmp : cmp;
    }
    if (lvalid)
    {
        return -1;
    }
    if (rvalid)
    {
        return 1;
    }
    return 0;
}

static void igTexture(const Texture* tex)
{
    if (!tex)
    {
        ASSERT(false);
        return;
    }

    const i32 width = tex->size.x;
    const i32 height = tex->size.y;
    if (!vkrTexTable_Exists(tex->slot))
    {
        ASSERT(false);
        return;
    }

    ImVec2 winSize = { 0 };
    igGetContentRegionAvail(&winSize);
    float winAspect = winSize.x / winSize.y;
    float texAspect = (float)width / (float)height;
    float scale;
    if (winAspect > texAspect)
    {
        scale = winSize.y / (float)height;
    }
    else
    {
        scale = winSize.x / (float)width;
    }

    const ImVec2 size = { width * scale, height * scale };
    const ImVec2 uv0 = { 0, 0 };
    const ImVec2 uv1 = { 1, 1 };
    const ImVec4 tint_col = { 1, 1, 1, 1 };
    const ImVec4 border_col = { 0, 0, 0, 0 };
    ImTextureID imtexid = { 0 };
    memcpy(&imtexid, &tex->slot, sizeof(tex->slot));
    SASSERT(sizeof(imtexid) >= sizeof(tex->slot));
    igImage(imtexid, size, uv0, uv1, tint_col, border_col);
}

ProfileMark(pm_OnGui, TextureSys_Gui)
void TextureSys_Gui(bool* pEnabled)
{
    ProfileBegin(pm_OnGui);

    i32 selection = gs_selection;

    if (igBegin("Textures", pEnabled, 0))
    {
        igInputText("Search", ARGS(gs_search), 0x0, NULL, NULL);

        const i32 width = ms_table.width;
        const Guid* pim_noalias names = ms_table.names;
        const Texture* pim_noalias textures = ms_table.values;
        const i32* pim_noalias refcounts = ms_table.refcounts;

        i32 bytesUsed = 0;
        for (i32 i = 0; i < width; ++i)
        {
            if (!Guid_IsNull(names[i]))
            {
                if (gs_search[0])
                {
                    char name[64] = { 0 };
                    Guid_GetName(names[i], ARGS(name));
                    if (!StrIStr(ARGS(name), gs_search))
                    {
                        continue;
                    }
                }
                int2 size = textures[i].size;
                i32 bytes = size.x * size.y * sizeof(u32);
                bytesUsed += bytes;
            }
        }

        igText("Bytes Used: %d", bytesUsed);

        if (igExButton("Clear Selection"))
        {
            selection = -1;
        }

        igSeparator();

        const char* const titles[] =
        {
            "Name",
            "Width",
            "Height",
            "References",
            "Select",
        };
        if (igExTableHeader(NELEM(titles), titles, &gs_cmpMode))
        {
            gs_revSort = !gs_revSort;
        }

        i32* indices = Temp_Calloc(sizeof(indices[0]) * width);
        for (i32 i = 0; i < width; ++i)
        {
            indices[i] = i;
        }
        QuickSort_Int(indices, width, CmpSlotFn, NULL);

        for (i32 i = 0; i < width; ++i)
        {
            i32 j = indices[i];
            ASSERT(j >= 0);
            ASSERT(j < width);
            Guid name = names[j];
            if (Guid_IsNull(name))
            {
                continue;
            }

            if (gs_search[0])
            {
                char namestr[64] = { 0 };
                Guid_GetName(name, ARGS(namestr));
                if (!StrIStr(ARGS(namestr), gs_search))
                {
                    continue;
                }
            }

            int2 size = textures[j].size;
            i32 refcount = refcounts[j];
            char namestr[PIM_PATH] = { 0 };
            if (!Guid_GetName(name, ARGS(namestr)))
            {
                Guid_Format(ARGS(namestr), name);
            }
            igText(namestr); igNextColumn();
            igText("%d", size.x); igNextColumn();
            igText("%d", size.y); igNextColumn();
            igText("%d", refcount); igNextColumn();
            const char* selectText = selection == j ? "Selected" : "Select";
            igPushIDInt(j);
            if (igExButton(selectText))
            {
                selection = j;
            }
            igPopID();
            igNextColumn();
        }

        igExTableFooter();
    }
    igEnd();

    if (pEnabled[0])
    {
        const i32 width = ms_table.width;
        const Guid* names = ms_table.names;
        const Texture* textures = ms_table.values;
        bool validSelection = (selection >= 0) &&
            (selection < width) &&
            !Guid_IsNull(names[selection]) &&
            textures[selection].texels;
        if (validSelection)
        {
            igBegin("Selected Texture", NULL, 0);
            igTexture(&textures[selection]);
            igEnd();
        }
        else
        {
            selection = -1;
        }
    }

    gs_selection = selection;

    ProfileEnd(pm_OnGui);
}
