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

static bool ms_once;
static table_t ms_table;

static u8 ms_palette[256 * 3];

static void EnsureInit(void)
{
    if (!ms_once)
    {
        ms_once = true;

        table_new(&ms_table, sizeof(texture_t));

        asset_t asset = { 0 };
        if (asset_get("gfx/palette.lmp", &asset))
        {
            const u8* palette = (const u8*)asset.pData;
            ASSERT(asset.length == sizeof(ms_palette));
            memcpy(ms_palette, palette, sizeof(ms_palette));
        }
    }
}

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
    ASSERT(tex);
    pim_free(tex->texels);
    memset(tex, 0, sizeof(*tex));
}

textureid_t texture_load(const char* path)
{
    EnsureInit();

    i32 width = 0;
    i32 height = 0;
    i32 channels = 0;
    u8* texels = stbi_load(path, &width, &height, &channels, 4);
    if (texels)
    {
        int2 size = { width, height };
        return texture_new(size, (u32*)texels, path);
    }
    return (textureid_t) { 0 };
}

textureid_t texture_new(int2 size, u32* texels, const char* name)
{
    EnsureInit();

    ASSERT(size.x > 0);
    ASSERT(size.y > 0);
    ASSERT(texels);

    genid id = { 0 };
    if (texels)
    {
        texture_t tex = { .size = size,.texels = texels };
        bool added = table_add(&ms_table, name, &tex, &id);
        ASSERT(added);
    }
    return ToTexId(id);
}

bool texture_exists(textureid_t id)
{
    EnsureInit();
    return IsCurrent(id);
}

void texture_retain(textureid_t id)
{
    EnsureInit();
    table_retain(&ms_table, ToGenId(id));
}

void texture_release(textureid_t id)
{
    EnsureInit();

    texture_t tex = {0};
    if (table_release(&ms_table, ToGenId(id), &tex))
    {
        pim_free(tex.texels);
    }
}

bool texture_get(textureid_t id, texture_t* dst)
{
    EnsureInit();
    return table_get(&ms_table, ToGenId(id), dst);
}

bool texture_set(textureid_t id, int2 size, u32* texels)
{
    EnsureInit();
    if (IsCurrent(id))
    {
        texture_t* texture = ms_table.values;
        texture += id.index;
        pim_free(texture->texels);
        texture->texels = texels;
        texture->size = size;
        return true;
    }
    return false;
}

textureid_t texture_lookup(const char* name)
{
    EnsureInit();
    return ToTexId(table_lookup(&ms_table, name));
}

// https://quakewiki.org/wiki/Quake_palette
textureid_t texture_unpalette(const u8* bytes, int2 size, const char* name)
{
    textureid_t id = texture_lookup(name);
    if (id.version)
    {
        return id;
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
            float4 lin = ColorToLinear(color);
            lin = DiffuseToAlbedo(lin);
            texels[i] = LinearToColor(lin);
        }
    }

    return texture_new(size, texels, name);
}

pim_inline float VEC_CALL SampleLuma(texture_t tex, int2 c)
{
    return f4_avglum(Wrap_c32(tex.texels, tex.size, c));
}

textureid_t texture_lumtonormal(textureid_t src, float scale, const char* name)
{
    textureid_t id = texture_lookup(name);
    if (id.version)
    {
        return id;
    }

    const float rcpScale = 1.0f / f1_max(scale, f16_eps);
    texture_t tex = { 0 };
    if (texture_get(src, &tex))
    {
        int2 size = tex.size;
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

                float dx = SampleLuma(tex, r) - SampleLuma(tex, l);
                float dy = SampleLuma(tex, u) - SampleLuma(tex, d);
                float z = (1.0f - sqrtf(dx * dx + dy * dy)) * rcpScale;
                float4 N = { dx, dy, z, 0.0f };
                u32 n = f4_rgba8(f4_unorm(f4_normalize3(N)));

                texels[i] = n;
            }
        }
        return texture_new(size, texels, name);
    }
    return (textureid_t) { 0 };
}
