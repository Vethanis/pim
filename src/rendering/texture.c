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
#include "io/fd.h"
#include <glad/glad.h>
#include <string.h>

static table_t ms_table;
static u8 ms_palette[256 * 3];

pim_inline genid ToGenId(textureid_t tid)
{
    genid gid;
    gid.index = tid.index;
    gid.version = tid.version;
    return gid;
}

pim_inline textureid_t ToTexId(genid gid)
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
    const guid_t* pim_noalias names = ms_table.names;
    texture_t* pim_noalias textures = ms_table.values;
    const i32 width = ms_table.width;
    for (i32 i = 0; i < width; ++i)
    {
        if (!guid_isnull(names[i]))
        {
            FreeTexture(textures + i);
        }
    }
    table_del(&ms_table);
}

bool texture_loadat(const char* path, textureid_t* idOut)
{
    i32 width = 0;
    i32 height = 0;
    i32 channels = 0;
    u8* pim_noalias texelsu8 = stbi_load(path, &width, &height, &channels, 4);
    if (texelsu8)
    {
        const i32 len = width * height;
        float4* pim_noalias texelsf4 = perm_malloc(sizeof(texelsf4[0]) * len);
        for (i32 i = 0; i < len; ++i)
        {
            texelsf4[i] = ColorToLinear(texelsu8[i]);
        }
        stbi_image_free(texelsu8);

        texture_t tex = { 0 };
        tex.size = (int2) { width, height };
        tex.texels = texelsf4;
        guid_t name = guid_str(path, guid_seed);
        return texture_new(&tex, name, idOut);
    }
    return false;
}

bool texture_new(texture_t* tex, guid_t name, textureid_t* idOut)
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

#define kTextureVersion 1

bool texture_save(textureid_t tid, guid_t* dst)
{
    if (texture_getname(tid, dst))
    {
        char filename[PIM_PATH] = { 0 };
        guid_fmt(ARGS(filename), *dst);
        StrCat(ARGS(filename), ".texture");
        fd_t fd = fd_create(filename);
        if (fd_isopen(fd))
        {
            const texture_t* textures = ms_table.values;
            const texture_t texture = textures[tid.index];
            const i32 len = texture.size.x * texture.size.y;
            const i32 version = kTextureVersion;
            fd_write(fd, &version, sizeof(version));
            fd_write(fd, &texture.size, sizeof(texture.size));
            fd_write(fd, texture.texels, sizeof(texture.texels[0]) * len);
            fd_close(&fd);
            return true;
        }
    }
    return false;
}

bool texture_load(guid_t name, textureid_t* dst)
{
    bool loaded = false;

    char filename[PIM_PATH] = { 0 };
    guid_fmt(ARGS(filename), name);
    StrCat(ARGS(filename), ".texture");
    fd_t fd = fd_open(filename, false);
    if (fd_isopen(fd))
    {
        texture_t texture = { 0 };
        i32 version = 0;
        fd_read(fd, &version, sizeof(version));
        if (version == kTextureVersion)
        {
            fd_read(fd, &texture.size, sizeof(texture.size));
            if ((texture.size.x > 0) && (texture.size.y > 0))
            {
                const i32 len = texture.size.x * texture.size.y;
                texture.texels = perm_malloc(sizeof(texture.texels[0]) * len);
                fd_read(fd, texture.texels, sizeof(texture.texels[0]) * len);
                loaded = texture_new(&texture, name, dst);
            }
        }
        fd_close(&fd);
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

// https://quakewiki.org/wiki/Quake_palette
bool texture_unpalette(
    const u8* bytes,
    int2 size,
    const char* name,
    textureid_t* albedoOut,
    textureid_t* romeOut,
    textureid_t* normalOut)
{
    char albedoName[PIM_PATH];
    char romeName[PIM_PATH];
    char normalName[PIM_PATH];

    SPrintf(ARGS(albedoName), "%s_albedo", name);
    SPrintf(ARGS(romeName), "%s_rome", name);
    SPrintf(ARGS(normalName), "%s_normal", name);

    guid_t albedoguid = guid_str(albedoName, guid_seed);
    guid_t romeguid = guid_str(romeName, guid_seed);
    guid_t normalguid = guid_str(normalName, guid_seed);

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

        float4* pim_noalias albedo = perm_malloc(len * sizeof(albedo[0]));
        float4* pim_noalias rome = perm_malloc(len * sizeof(rome[0]));
        float4* pim_noalias normal = perm_malloc(len * sizeof(normal[0]));

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
            albedo[i] = f4_saturate(linear);
        }

        // TODO: make a node graph tool, setup some rules
        // for surface properties for each texture.
        for (i32 i = 0; i < len; ++i)
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

            rome[i] = f4_saturate(f4_v(roughness, occlusion, metallic, emission));
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
                float4 N = { dx, dy, z, 1.0f };
                N = f4_normalize3(N);
                //u32 n = f4_rgba8(f4_unorm(f4_normalize3(N)));
                //n |= 0xff << 24;

                normal[i] = N;
            }
        }

        pim_free(gray);
        gray = NULL;

        texture_t albedoMap = { 0 };
        albedoMap.size = size;
        albedoMap.texels = albedo;
        albedoAdded = texture_new(&albedoMap, albedoguid, albedoOut);

        texture_t romeMap = { 0 };
        romeMap.size = size;
        romeMap.texels = rome;
        romeAdded = texture_new(&romeMap, romeguid, romeOut);

        texture_t normalMap = { 0 };
        normalMap.size = size;
        normalMap.texels = normal;
        normalAdded = texture_new(&normalMap, normalguid, normalOut);
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
            cmp = guid_cmp(lhs, rhs);
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
    const float4* data = tex->texels;
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
        GL_FLOAT,                   // type
        data);                      // data
    ASSERT(!glGetError());

    glBindTexture(GL_TEXTURE_2D, 0);
    ASSERT(!glGetError());


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
        const i32 width = ms_table.width;
        const guid_t* pim_noalias names = ms_table.names;
        const texture_t* pim_noalias textures = ms_table.values;
        const i32* pim_noalias refcounts = ms_table.refcounts;

        i32 bytesUsed = 0;
        for (i32 i = 0; i < width; ++i)
        {
            if (!guid_isnull(names[i]))
            {
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
            guid_t name = names[j];
            if (guid_isnull(name))
            {
                continue;
            }

            int2 size = textures[j].size;
            i32 refcount = refcounts[j];
            char namestr[PIM_PATH] = { 0 };
            guid_fmt(ARGS(namestr), name);
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
