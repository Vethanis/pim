#include "rendering/texture.h"
#include "allocator/allocator.h"
#include "containers/table.h"
#include "math/color.h"
#include "math/blending.h"
#include "rendering/sampler.h"
#include "rendering/material.h"
#include "assets/asset_system.h"
#include "quake/q_bspfile.h"
#include "stb/stb_image.h"
#include "common/stringutil.h"
#include "ui/cimgui.h"
#include "ui/cimgui_ext.h"
#include "common/sort.h"
#include "common/profiler.h"
#include "io/fstr.h"
#include "threading/task.h"
#include "rendering/vulkan/vkr_texture.h"
#include "rendering/vulkan/vkr_textable.h"
#include "assets/crate.h"
#include <string.h>

static table_t ms_table;
static u8 ms_palette[256 * 3];

static genid ToGenId(textureid_t tid)
{
    genid gid;
    gid.index = tid.index;
    gid.version = tid.version;
    return gid;
}

static textureid_t ToTexId(genid gid)
{
    textureid_t tid;
    tid.index = gid.index;
    tid.version = gid.version;
    return tid;
}

static bool IsCurrent(textureid_t id)
{
    return table_exists(&ms_table, ToGenId(id));
}

static void FreeTexture(texture_t* tex)
{
    pim_free(tex->texels);
    vkrTexTable_Free(tex->slot);
    memset(tex, 0, sizeof(*tex));
}

const table_t* texture_table(void)
{
    return &ms_table;
}

void texture_sys_init(void)
{
    table_new(&ms_table, sizeof(texture_t));

    asset_t asset = { 0 };
    if (asset_get("gfx/palette.lmp", &asset))
    {
        const u8* palette = (const u8*)asset.pData;
        ASSERT(asset.length == sizeof(ms_palette));
        memcpy(ms_palette, palette, sizeof(ms_palette));
    }
}

void texture_sys_update(void)
{

}

void texture_sys_shutdown(void)
{
    texture_t* pim_noalias textures = ms_table.values;
    const i32 width = ms_table.width;
    for (i32 i = 0; i < width; ++i)
    {
        if (textures[i].texels)
        {
            FreeTexture(textures + i);
        }
    }
    table_del(&ms_table);
}

bool texture_loadat(const char* path, VkFormat format, textureid_t* idOut)
{
    i32 width = 0;
    i32 height = 0;
    i32 channels = 0;
    // STBI_MALLOC => pim_malloc(EAlloc_Texture, bytes)
    u32* pim_noalias texels = (u32*)stbi_load(path, &width, &height, &channels, 4);
    if (texels)
    {
        texture_t tex = { 0 };
        tex.size = (int2) { width, height };
        tex.texels = texels;
        guid_t name = guid_str(path);
        return texture_new(&tex, format, name, idOut);
    }
    return false;
}

bool texture_new(
    texture_t* tex,
    VkFormat format,
    guid_t name,
    textureid_t* idOut)
{
    ASSERT(tex);
    ASSERT(idOut);
    ASSERT(tex->size.x > 0);
    ASSERT(tex->size.y > 0);
    ASSERT(tex->texels);

    bool added = false;
    genid id = { 0, 0 };
    if (tex->texels)
    {
        tex->format = format;
        if (table_find(&ms_table, name, &id))
        {
            table_retain(&ms_table, id);
        }
        else
        {
            i32 width = tex->size.x;
            i32 height = tex->size.y;
            tex->slot = vkrTexTable_Alloc(
                VK_IMAGE_VIEW_TYPE_2D,
                format,
                width,
                height,
                1, // depth
                1, // layers
                true); // mips
            i32 bytes = (width * height * vkrFormatToBpp(format)) / 8;
            VkFence fence = vkrTexTable_Upload(tex->slot, 0, tex->texels, bytes);
            ASSERT(fence);
            if (fence)
            {
                added = table_add(&ms_table, name, tex, &id);
                ASSERT(added);
            }
        }
    }
    *idOut = ToTexId(id);
    if (!added)
    {
        FreeTexture(tex);
    }
    return added;
}

bool texture_exists(textureid_t id)
{
    return IsCurrent(id);
}

void texture_retain(textureid_t id)
{
    table_retain(&ms_table, ToGenId(id));
}

void texture_release(textureid_t id)
{
    texture_t tex = { 0 };
    if (table_release(&ms_table, ToGenId(id), &tex))
    {
        FreeTexture(&tex);
    }
}

texture_t* texture_get(textureid_t id)
{
    return table_get(&ms_table, ToGenId(id));
}

bool texture_find(guid_t name, textureid_t* idOut)
{
    ASSERT(idOut);
    genid id;
    bool found = table_find(&ms_table, name, &id);
    *idOut = ToTexId(id);
    return found;
}

bool texture_getname(textureid_t tid, guid_t* nameOut)
{
    genid gid = ToGenId(tid);
    return table_getname(&ms_table, gid, nameOut);
}

bool texture_save(crate_t* crate, textureid_t tid, guid_t* dst)
{
    bool wasSet = false;
    if (texture_getname(tid, dst))
    {
        const texture_t* textures = ms_table.values;
        const texture_t texture = textures[tid.index];
        const i32 len = texture.size.x * texture.size.y;
        const i32 bpp = vkrFormatToBpp(texture.format);
        const i32 texelBytes = (bpp * len) / 8;
        const i32 hdrBytes = sizeof(dtexture_t);
        dtexture_t* dtexture = tex_malloc(hdrBytes + texelBytes);
        if (dtexture)
        {
            dtexture->version = kTextureVersion;
            dtexture->format = texture.format;
            dtexture->size = texture.size;
            guid_get_name(*dst, ARGS(dtexture->name));
            memcpy(dtexture + 1, texture.texels, texelBytes);
            wasSet = crate_set(crate, *dst, dtexture, hdrBytes + texelBytes);
        }
        pim_free(dtexture);
    }
    return wasSet;
}

bool texture_load(crate_t* crate, guid_t name, textureid_t* dst)
{
    bool loaded = false;
    texture_t texture = { 0 };
    i32 offset = 0;
    i32 size = 0;
    if (crate_stat(crate, name, &offset, &size))
    {
        dtexture_t* dtexture = tex_malloc(size);
        if (dtexture && crate_get(crate, name, dtexture, size))
        {
            if (dtexture->version == kTextureVersion)
            {
                const VkFormat format = dtexture->format;
                const i32 bpp = vkrFormatToBpp(format);
                texture.size = dtexture->size;
                if ((texture.size.x > 0) && (texture.size.y > 0) && (bpp > 0))
                {
                    guid_str(dtexture->name);
                    const i32 len = texture.size.x * texture.size.y;
                    const i32 texelBytes = (bpp * len) / 8;
                    memmove(dtexture, dtexture + 1, texelBytes);
                    texture.texels = (u32*)dtexture;
                    dtexture = NULL;
                    loaded = texture_new(&texture, format, name, dst);
                }
            }
        }
        pim_free(dtexture);
    }
    return loaded;
}

typedef enum
{
    PalRow_White,
    PalRow_Brown,
    PalRow_LightBlue,
    PalRow_Green,
    PalRow_Red,
    PalRow_Orange,
    PalRow_Gold,
    PalRow_Peach,
    PalRow_Purple,
    PalRow_Magenta,
    PalRow_Tan,
    PalRow_LightGreen,
    PalRow_Yellow,
    PalRow_Blue,
    PalRow_Fire,
    PalRow_Brights,

    PalRow_COUNT
} PalRow;

pim_inline u32 DecodeTexel(u8 encoded)
{
    const u8* pim_noalias palette = ms_palette;
    u32 r = palette[encoded * 3 + 0];
    u32 g = palette[encoded * 3 + 1];
    u32 b = palette[encoded * 3 + 2];
    u32 color = r | (g << 8) | (b << 16) | (0xff << 24);
    return color;
}

pim_inline float DecodeEmission(u8 encoded, bool isLight)
{
    u8 row = encoded / 16;
    u8 column = encoded % 16;
    switch (row)
    {
    case PalRow_White:
        return column >= 13 ? 1.0f : 0.0f;
    case PalRow_Gold:
        return column >= 13 ? 1.0f : 0.0f;
    case PalRow_Yellow:
        return column <= 2 ? 1.0f : 0.0f;
    case PalRow_Blue:
        return column <= 2 ? 1.0f : 0.0f;
    case PalRow_Fire:
        return 1.0f;
    case PalRow_Brights:
        return 1.0f;
    }
    if (isLight)
    {
        if (row < 8)
        {
            return column >= 12 ? 1.0f : 0.0f;
        }
        if (row >= PalRow_Fire)
        {
            return 1.0f;
        }
        return column <= 3 ? 1.0f : 0.0f;
    }
    return 0.0f;
}

typedef struct task_Unpalette
{
    task_t task;
    float4 flatRome;
    int2 size;
    float2 min;
    float2 max;
    const u8* bytes;
    u32* albedo;
    u32* rome;
    u32* normal;
    float2* gray;
    matflag_t flags;
    bool fullEmit;
    bool isLight;
} task_Unpalette;

static void UnpaletteStep1Fn(void* pbase, i32 begin, i32 end)
{
    task_Unpalette* task = pbase;
    const matflag_t flags = task->flags;
    const u8* pim_noalias bytes = task->bytes;
    u32* pim_noalias albedo = task->albedo;
    float2* pim_noalias gray = task->gray;

    for (i32 i = begin; i < end; ++i)
    {
        u32 color = DecodeTexel(bytes[i]);
        float4 diffuse = ColorToLinear(color);
        float4 linear = DiffuseToAlbedo(diffuse);
        linear.w = 1.0f;
        gray[i] = f2_v(f4_avglum(diffuse), f4_avglum(linear));
        albedo[i] = LinearToColor(linear);
    }
}

static void UnpaletteStep2Fn(void* pbase)
{
    task_Unpalette* task = pbase;
    const int2 size = task->size;
    const i32 len = size.x * size.y;
    const float2* pim_noalias gray = task->gray;

    float2 min = f2_1;
    float2 max = f2_0;
    for (i32 i = 0; i < len; ++i)
    {
        min = f2_min(min, gray[i]);
        max = f2_max(max, gray[i]);
    }
    task->min = min;
    task->max = max;
}

static void UnpaletteStep3Fn(void* pbase, i32 begin, i32 end)
{
    task_Unpalette* task = pbase;
    const float4 flatRome = task->flatRome;
    const int2 size = task->size;
    const float2 min = task->min;
    const float2 max = task->max;
    const u8* pim_noalias bytes = task->bytes;
    const float2* pim_noalias gray = task->gray;
    u32* pim_noalias rome = task->rome;
    u32* pim_noalias normal = task->normal;
    const bool fullEmit = task->fullEmit;
    const bool isLight = task->isLight;

    for (i32 i = begin; i < end; ++i)
    {
        u8 encoded = bytes[i];
        float2 grayscale = gray[i];
        float2 t = f2_smoothstep(min, max, grayscale);
        float roughness = f1_lerp(1.0f, 0.9f, t.y);
        float occlusion = f1_lerp(0.9f, 1.0f, t.y);
        float metallic = 1.0f;
        float emission;
        if (fullEmit)
        {
            emission = 1.0f;
        }
        else
        {
            emission = DecodeEmission(encoded, isLight);
        }
        float4 romeValue = f4_v(roughness, occlusion, metallic, emission);
        romeValue = f4_mul(romeValue, flatRome);
        rome[i] = LinearToColor(romeValue);
    }

    for (i32 i = begin; i < end; ++i)
    {
        i32 x = i % size.x;
        i32 y = i / size.x;
        float r = gray[Wrap(size, i2_v(x + 1, y + 0))].x;
        float l = gray[Wrap(size, i2_v(x - 1, y + 0))].x;
        float u = gray[Wrap(size, i2_v(x + 0, y - 1))].x;
        float d = gray[Wrap(size, i2_v(x + 0, y + 1))].x;

        r = f1_smoothstep(min.x, max.x, r);
        l = f1_smoothstep(min.x, max.x, l);
        u = f1_smoothstep(min.x, max.x, u);
        d = f1_smoothstep(min.x, max.x, d);

        float dx = r - l;
        float dy = u - d;
        const float z = 2.0f;
        float4 N = { -dx, -dy, z, 1.0f };
        normal[i] = DirectionToColor(N);
    }
}

// https://quakewiki.org/wiki/Quake_palette
bool texture_unpalette(
    u8 const *const pim_noalias bytes,
    int2 size,
    const char* name,
    u32 matflags,
    float4 flatRome,
    textureid_t *const albedoOut,
    textureid_t *const romeOut,
    textureid_t *const normalOut)
{
    char albedoName[PIM_PATH] = { 0 };
    char romeName[PIM_PATH] = { 0 };
    char normalName[PIM_PATH] = { 0 };

    SPrintf(ARGS(albedoName), "%s_albedo", name);
    SPrintf(ARGS(romeName), "%s_rome", name);
    SPrintf(ARGS(normalName), "%s_normal", name);

    guid_t albedoguid = guid_str(albedoName);
    guid_t romeguid = guid_str(romeName);
    guid_t normalguid = guid_str(normalName);

    bool albedoExists = false;
    bool romeExists = false;
    bool normalExists = false;

    bool albedoAdded = false;
    bool romeAdded = false;
    bool normalAdded = false;

    if (texture_find(albedoguid, albedoOut))
    {
        albedoExists = true;
        texture_retain(*albedoOut);
    }
    if (texture_find(romeguid, romeOut))
    {
        romeExists = true;
        texture_retain(*romeOut);
    }
    if (texture_find(normalguid, normalOut))
    {
        normalExists = true;
        texture_retain(*normalOut);
    }

    const i32 len = size.x * size.y;
    if (!albedoExists || !romeExists || !normalExists)
    {
        const bool isSky = StrIStr(name, 16, "sky");
        const bool isTeleport = StrIStr(name, 16, "teleport");
        const bool isWindow = StrIStr(name, 16, "window");
        const bool isLight = StrIStr(name, 16, "light");
        const bool fullEmit = isSky || isTeleport || isWindow;

        u32* pim_noalias albedo = tex_malloc(len * sizeof(albedo[0]));
        u32* pim_noalias rome = tex_malloc(len * sizeof(rome[0]));
        u32* pim_noalias normal = tex_malloc(len * sizeof(normal[0]));
        float2* pim_noalias gray = tex_malloc(len * sizeof(gray[0]));

        task_Unpalette* tasks = tmp_calloc(sizeof(tasks[0]) * 3);
        tasks[0].albedo = albedo;
        tasks[0].bytes = bytes;
        tasks[0].flags = matflags;
        tasks[0].flatRome = flatRome;
        tasks[0].fullEmit = fullEmit;
        tasks[0].gray = gray;
        tasks[0].isLight = isLight;
        tasks[0].max = f2_0;
        tasks[0].min = f2_1;
        tasks[0].normal = normal;
        tasks[0].rome = rome;
        tasks[0].size = size;

        // cannot reuse same task across invocations, the signalling state gets corrupted
        task_run(&tasks[0].task, UnpaletteStep1Fn, len);
        tasks[1] = tasks[0];
        tasks[1].task = (task_t) { 0 };
        UnpaletteStep2Fn(&tasks[1].task);
        tasks[2] = tasks[1];
        tasks[2].task = (task_t) { 0 };
        task_run(&tasks[2].task, UnpaletteStep3Fn, len);

        pim_free(gray);
        gray = NULL;

        texture_t albedoMap = { 0 };
        albedoMap.size = size;
        albedoMap.texels = albedo;
        albedoAdded = texture_new(
            &albedoMap, VK_FORMAT_R8G8B8A8_SRGB, albedoguid, albedoOut);

        texture_t romeMap = { 0 };
        romeMap.size = size;
        romeMap.texels = rome;
        romeAdded = texture_new(
            &romeMap, VK_FORMAT_R8G8B8A8_SRGB, romeguid, romeOut);

        texture_t normalMap = { 0 };
        normalMap.size = size;
        normalMap.texels = normal;
        normalAdded = texture_new(
            &normalMap, VK_FORMAT_R8G8B8A8_UNORM, normalguid, normalOut);
    }

    return albedoAdded && romeAdded && normalAdded;
}

// ----------------------------------------------------------------------------

static bool gs_revSort;
static i32 gs_cmpMode;
static i32 gs_selection;
static char gs_search[PIM_PATH];

static i32 CmpSlotFn(i32 ilhs, i32 irhs, void* usr)
{
    const i32 width = ms_table.width;
    const guid_t* pim_noalias names = ms_table.names;
    const texture_t* pim_noalias textures = ms_table.values;
    const i32* pim_noalias refcounts = ms_table.refcounts;

    guid_t lhs = names[ilhs];
    guid_t rhs = names[irhs];
    bool lvalid = !guid_isnull(lhs);
    bool rvalid = !guid_isnull(rhs);
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
            guid_get_name(lhs, ARGS(lname));
            guid_get_name(rhs, ARGS(rname));
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

static void igTexture(const texture_t* tex)
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
    igImage(*(u32*)&tex->slot, size, uv0, uv1, tint_col, border_col);
}

ProfileMark(pm_OnGui, texture_sys_gui)
void texture_sys_gui(bool* pEnabled)
{
    ProfileBegin(pm_OnGui);

    i32 selection = gs_selection;

    if (igBegin("Textures", pEnabled, 0))
    {
        igInputText("Search", ARGS(gs_search), 0x0, NULL, NULL);

        const i32 width = ms_table.width;
        const guid_t* pim_noalias names = ms_table.names;
        const texture_t* pim_noalias textures = ms_table.values;
        const i32* pim_noalias refcounts = ms_table.refcounts;

        i32 bytesUsed = 0;
        for (i32 i = 0; i < width; ++i)
        {
            if (!guid_isnull(names[i]))
            {
                if (gs_search[0])
                {
                    char name[64] = { 0 };
                    guid_get_name(names[i], ARGS(name));
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

        if (igButton("Clear Selection"))
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
        if (igTableHeader(NELEM(titles), titles, &gs_cmpMode))
        {
            gs_revSort = !gs_revSort;
        }

        i32* indices = tmp_calloc(sizeof(indices[0]) * width);
        for (i32 i = 0; i < width; ++i)
        {
            indices[i] = i;
        }
        sort_i32(indices, width, CmpSlotFn, NULL);

        for (i32 i = 0; i < width; ++i)
        {
            i32 j = indices[i];
            ASSERT(j >= 0);
            ASSERT(j < width);
            guid_t name = names[j];
            if (guid_isnull(name))
            {
                continue;
            }

            if (gs_search[0])
            {
                char namestr[64] = { 0 };
                guid_get_name(name, ARGS(namestr));
                if (!StrIStr(ARGS(namestr), gs_search))
                {
                    continue;
                }
            }

            int2 size = textures[j].size;
            i32 refcount = refcounts[j];
            char namestr[PIM_PATH] = { 0 };
            if (!guid_get_name(name, ARGS(namestr)))
            {
                guid_fmt(ARGS(namestr), name);
            }
            igText(namestr); igNextColumn();
            igText("%d", size.x); igNextColumn();
            igText("%d", size.y); igNextColumn();
            igText("%d", refcount); igNextColumn();
            const char* selectText = selection == j ? "Selected" : "Select";
            igPushIDInt(j);
            if (igButton(selectText))
            {
                selection = j;
            }
            igPopID();
            igNextColumn();
        }

        igTableFooter();
    }
    igEnd();

    if (pEnabled[0])
    {
        const i32 width = ms_table.width;
        const guid_t* names = ms_table.names;
        const texture_t* textures = ms_table.values;
        bool validSelection = (selection >= 0) &&
            (selection < width) &&
            !guid_isnull(names[selection]) &&
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
