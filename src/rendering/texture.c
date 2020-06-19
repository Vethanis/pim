#include "rendering/texture.h"
#include "allocator/allocator.h"
#include "containers/table.h"
#include "math/color.h"
#include "math/blending.h"
#include "rendering/sampler.h"
#include "assets/asset_system.h"
#include "quake/q_bspfile.h"
#include "stb/stb_image.h"
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

// https://quakewiki.org/wiki/Quake_palette
bool texture_unpalette(const u8* bytes, int2 size, const char* name, textureid_t* idOut)
{
    if (texture_find(name, idOut))
    {
        texture_retain(*idOut);
        return false;
    }

    u32* texels = perm_malloc(size.x * size.y * sizeof(texels[0]));
    for (i32 y = 0; y < size.y; ++y)
    {
        for (i32 x = 0; x < size.x; ++x)
        {
            i32 i = x + y * size.x;
            u8 encoded = bytes[i];
            u32 r = ms_palette[encoded * 3 + 0];
            u32 g = ms_palette[encoded * 3 + 1];
            u32 b = ms_palette[encoded * 3 + 2];
            u32 color = r | (g << 8) | (b << 16);
            texels[i] = color;
        }
    }

    texture_t tex = { 0 };
    tex.size = size;
    tex.texels = texels;
    return texture_new(&tex, name, idOut);
}

bool texture_diffuse_to_albedo(textureid_t id)
{
    texture_t tex;
    if (texture_get(id, &tex))
    {
        const i32 len = tex.size.x * tex.size.y;
        for (i32 i = 0; i < len; ++i)
        {
            u32 diffuseColor = tex.texels[i];
            float4 diffuse = ColorToLinear(diffuseColor);
            float4 albedo = DiffuseToAlbedo(diffuse);
            u32 albedoColor = LinearToColor(albedo);
            tex.texels[i] = albedoColor;
        }
        return true;
    }
    return false;
}

pim_inline float VEC_CALL SampleLuma(texture_t tex, int2 c)
{
    return f4_perlum(Wrap_c32(tex.texels, tex.size, c));
}

bool texture_lumtonormal(textureid_t src, float scale, const char* name, textureid_t* idOut)
{
    if (texture_find(name, idOut))
    {
        texture_retain(*idOut);
        return false;
    }

    const float rcpScale = 1.0f / f1_max(scale, kEpsilon);
    texture_t srcTex = { 0 };
    if (texture_get(src, &srcTex))
    {
        int2 size = srcTex.size;
        u32* texels = perm_malloc(size.x * size.y * sizeof(texels[0]));
        for (i32 y = 0; y < size.y; ++y)
        {
            for (i32 x = 0; x < size.x; ++x)
            {
                i32 i = x + y * size.x;
                int2 r = { x + 1, y };
                int2 l = { x - 1, y };
                int2 u = { x, y + 1 };
                int2 d = { x, y - 1 };

                float dx = SampleLuma(srcTex, r) - SampleLuma(srcTex, l);
                float dy = SampleLuma(srcTex, u) - SampleLuma(srcTex, d);
                float z = (1.0f - sqrtf(dx * dx + dy * dy)) * rcpScale;
                float4 N = { dx, dy, z, 0.0f };
                u32 n = f4_rgba8(f4_unorm(f4_normalize3(N)));

                texels[i] = n;
            }
        }
        texture_t dstTex = { 0 };
        dstTex.size = size;
        dstTex.texels = texels;
        return texture_new(&dstTex, name, idOut);
    }
    return false;
}
