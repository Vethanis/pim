#include "rendering/texture.h"
#include "allocator/allocator.h"
#include "containers/table.h"
#include "math/color.h"
#include "math/blending.h"
#include "rendering/sampler.h"
#include "assets/asset_system.h"
#include "quake/q_bspfile.h"
#include "stb/stb_image.h"
#include "common/stringutil.h"
#include "ui/cimgui.h"
#include "ui/cimgui_ext.h"
#include "common/sort.h"
#include "common/profiler.h"
#include <glad/glad.h>
#include <string.h>

static table_t ms_table;
static u8 ms_palette[256 * 3];

static genid ToGenId(textureid_t id)
{
    return (genid) { .index = id.index, .version = id.version };
}

static textureid_t ToTexId(genid id)
{
    return (textureid_t) { .index = id.index, .version = id.version };
}

static bool IsCurrent(textureid_t id)
{
    return table_exists(&ms_table, ToGenId(id));
}

static void FreeTexture(texture_t* tex)
{
    pim_free(tex->texels);
    memset(tex, 0, sizeof(*tex));
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
    i32 width = ms_table.width;
    const char** names = ms_table.names;
    texture_t* textures = ms_table.values;
    for (i32 i = 0; i < ms_table.width; ++i)
    {
        if (names[i])
        {
            FreeTexture(textures + i);
        }
    }
    table_del(&ms_table);
}

bool texture_load(const char* path, textureid_t* idOut)
{
    i32 width = 0;
    i32 height = 0;
    i32 channels = 0;
    u8* texels = stbi_load(path, &width, &height, &channels, 4);
    if (texels)
    {
        int2 size = { width, height };
        texture_t tex = { 0 };
        tex.size = size;
        tex.texels = (u32*)texels;
        return texture_new(&tex, path, idOut);
    }
    return false;
}

bool texture_new(texture_t* tex, const char* name, textureid_t* idOut)
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
        added = table_add(&ms_table, name, tex, &id);
        ASSERT(added);
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

bool texture_get(textureid_t id, texture_t* dst)
{
    return table_get(&ms_table, ToGenId(id), dst);
}

bool texture_set(textureid_t id, texture_t* src)
{
    ASSERT(src);
    if (IsCurrent(id))
    {
        texture_t* dst = ms_table.values;
        dst += id.index;
        FreeTexture(dst);
        memcpy(dst, src, sizeof(*dst));
        memset(src, 0, sizeof(*src));
        return true;
    }
    return false;
}

bool texture_find(const char* name, textureid_t* idOut)
{
    ASSERT(idOut);
    genid id;
    bool found = table_find(&ms_table, name, &id);
    *idOut = ToTexId(id);
    return found;
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
    const u32 a = 0xff;
    u32 color = r | (g << 8) | (b << 16) | (a << 24);
    return color;
}

pim_inline float DecodeEmission(u8 encoded)
{
    u8 row = encoded / 16;
    u8 column = encoded % 16;
    if (row == PalRow_Fire)
    {
        return (0.5f + column) / 16.0f;
    }
    if (row == PalRow_Brights)
    {
        return 1.0f;
    }
    return 0.0f;
}

// https://quakewiki.org/wiki/Quake_palette
bool texture_unpalette(
    const u8* bytes,
    int2 size,
    const char* name,
    textureid_t* albedoOut,
    textureid_t* romeOut,
    textureid_t* normalOut)
{
    const bool isSky = StrIStr(name, 16, "sky");
    const bool isTeleport = StrIStr(name, 16, "teleport");
    const bool isWindow = StrIStr(name, 16, "window");

    bool albedoExists = false;
    bool albedoAdded = false;
    char albedoName[PIM_PATH];
    SPrintf(ARGS(albedoName), "%s_albedo", name);
    if (texture_find(albedoName, albedoOut))
    {
        albedoExists = true;
        texture_retain(*albedoOut);
    }

    bool romeExists = false;
    bool romeAdded = false;
    char romeName[PIM_PATH];
    SPrintf(ARGS(romeName), "%s_rome", name);
    if (texture_find(romeName, romeOut))
    {
        romeExists = true;
        texture_retain(*romeOut);
    }

    bool normalExists = false;
    bool normalAdded = false;
    char normalName[PIM_PATH];
    SPrintf(ARGS(normalName), "%s_normal", name);
    if (texture_find(normalName, normalOut))
    {
        normalExists = true;
        texture_retain(*normalOut);
    }

    const i32 len = size.x * size.y;
    if (!albedoExists || !romeExists || !normalExists)
    {
        u32* pim_noalias albedo = perm_malloc(len * sizeof(albedo[0]));
        u32* pim_noalias rome = perm_malloc(len * sizeof(rome[0]));
        u32* pim_noalias normal = perm_malloc(len * sizeof(normal[0]));

        float2* pim_noalias gray = perm_malloc(len * sizeof(gray[0]));

        float2 min = f2_1;
        float2 max = f2_0;
        for (i32 i = 0; i < len; ++i)
        {
            u32 color = DecodeTexel(bytes[i]);
            float4 diffuse = ColorToLinear(color);
            float4 linear = DiffuseToAlbedo(diffuse);
            linear.w = 1.0f;
            float2 grayscale = f2_v(f4_perlum(diffuse), f4_perlum(linear));
            min = f2_min(min, grayscale);
            max = f2_max(max, grayscale);

            gray[i] = grayscale;
            albedo[i] = LinearToColor(linear);
        }

        // TODO: make a node graph tool, setup some rules
        // for surface properties for each texture.
        for (i32 i = 0; i < len; ++i)
        {
            u8 encoded = bytes[i];
            float2 grayscale = gray[i];
            float2 t = f2_smoothstep(min, max, grayscale);

            float roughness;
            float occlusion;
            float metallic;
            float emission;
            if (isWindow)
            {
                roughness = 0.25f;
                occlusion = 1.0f;
                metallic = 0.0f;
                emission = 0.5f + 0.5f * t.y;
            }
            else if (isTeleport)
            {
                roughness = 1.0f;
                occlusion = 0.0f;
                metallic = 0.0f;
                emission = 0.5f + 0.5f * t.x;
            }
            else if (isSky)
            {
                roughness = 1.0f;
                occlusion = 0.0f;
                metallic = 0.0f;
                emission = t.y;
            }
            else
            {
                roughness = f1_lerp(1.0f, 0.9f, t.y);
                occlusion = f1_lerp(0.9f, 1.0f, t.y);
                metallic = 1.0f;
                emission = DecodeEmission(encoded);
            }

            rome[i] = LinearToColor(f4_v(roughness, occlusion, metallic, emission));
        }

        for (i32 y = 0; y < size.y; ++y)
        {
            for (i32 x = 0; x < size.x; ++x)
            {
                i32 i = x + y * size.x;
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
                float z = 2.0f;
                float4 N = { dx, dy, z, 0.0f };
                u32 n = f4_rgba8(f4_unorm(f4_normalize3(N)));
                n |= 0xff << 24;

                normal[i] = n;
            }
        }

        pim_free(gray);
        gray = NULL;

        texture_t albedoMap = { 0 };
        albedoMap.size = size;
        albedoMap.texels = albedo;
        albedoAdded = texture_new(&albedoMap, albedoName, albedoOut);

        texture_t romeMap = { 0 };
        romeMap.size = size;
        romeMap.texels = rome;
        romeAdded = texture_new(&romeMap, romeName, romeOut);

        texture_t normalMap = { 0 };
        normalMap.size = size;
        normalMap.texels = normal;
        normalAdded = texture_new(&normalMap, normalName, normalOut);
    }

    return albedoAdded && romeAdded && normalAdded;
}

// ----------------------------------------------------------------------------

static bool gs_revSort;
static i32 gs_cmpMode;
static i32 gs_texHandle;
static i32 gs_selection;
static char gs_search[PIM_PATH];

static i32 CmpSlotFn(i32 ilhs, i32 irhs, void* usr)
{
    const i32 width = ms_table.width;
    const char** names = ms_table.names;
    const texture_t* textures = ms_table.values;
    const i32* refcounts = ms_table.refcounts;

    if (names[ilhs] && names[irhs])
    {
        i32 cmp = 0;
        switch (gs_cmpMode)
        {
        default:
        case 0:
            cmp = StrCmp(names[ilhs], PIM_PATH, names[irhs]);
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
    if (names[ilhs])
    {
        return -1;
    }
    if (names[irhs])
    {
        return 1;
    }
    return 0;
}

static void igTexture(const texture_t* tex)
{
    ASSERT(!glGetError());

    if (!gs_texHandle)
    {
        glGenTextures(1, &gs_texHandle);
        ASSERT(!glGetError());
    }

    if (!tex)
    {
        ASSERT(false);
        return;
    }

    const i32 width = tex->size.x;
    const i32 height = tex->size.y;
    const u32* data = tex->texels;
    if (!data)
    {
        ASSERT(false);
        return;
    }

    glBindTexture(GL_TEXTURE_2D, gs_texHandle);
    ASSERT(!glGetError());

    ASSERT(!glGetError());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    ASSERT(!glGetError());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    ASSERT(!glGetError());

    glTexImage2D(
        GL_TEXTURE_2D,              // target
        0,                          // level
        GL_RGBA,                    // internalformat
        width,                      // width
        height,                     // height
        GL_FALSE,                   // border
        GL_RGBA,                    // format
        GL_UNSIGNED_BYTE,           // type
        data);                      // data
    ASSERT(!glGetError());

    glBindTexture(GL_TEXTURE_2D, 0);
    ASSERT(!glGetError());


    ImVec2 winSize = {0};
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

    const isize id = gs_texHandle;
    const ImVec2 size = { width * scale, height * scale };
    const ImVec2 uv0 = { 0, 0 };
    const ImVec2 uv1 = { 1, 1 };
    const ImVec4 tint_col = { 1, 1, 1, 1 };
    const ImVec4 border_col = { 0, 0, 0, 0 };
    igImage((ImTextureID)id, size, uv0, uv1, tint_col, border_col);
}

ProfileMark(pm_OnGui, texture_sys_gui)
void texture_sys_gui(bool* pEnabled)
{
    ProfileBegin(pm_OnGui);

    i32 selection = gs_selection;

    if (igBegin("Textures", pEnabled, 0))
    {
        igInputText("Search", gs_search, NELEM(gs_search), 0, 0, 0);
        const char* search = gs_search;

        const i32 width = ms_table.width;
        const char** names = ms_table.names;
        const texture_t* textures = ms_table.values;
        const i32* refcounts = ms_table.refcounts;

        i32 bytesUsed = 0;
        for (i32 i = 0; i < width; ++i)
        {
            const char* name = names[i];
            if (name)
            {
                if (search[0] && !StrIStr(name, PIM_PATH, search))
                {
                    continue;
                }
                int2 size = textures[i].size;
                bytesUsed += size.x * size.y * sizeof(u32);
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
            const char* name = names[j];
            if (!name)
            {
                continue;
            }
            if (search[0] && !StrIStr(name, PIM_PATH, search))
            {
                continue;
            }

            int2 size = textures[j].size;
            i32 refcount = refcounts[j];
            igText(name); igNextColumn();
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
        const char** names = ms_table.names;
        const texture_t* textures = ms_table.values;
        bool validSelection = (selection >= 0) &&
            (selection < width) &&
            names[selection] &&
            textures[selection].texels;
        if (validSelection)
        {
            igBegin("Selected Texture", NULL, 0);
            igTexture(textures + selection);
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
