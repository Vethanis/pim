#include "rendering/texture.h"
#include "allocator/allocator.h"
#include "common/atomics.h"
#include "containers/dict.h"
#include "common/guid.h"
#include "common/fnv1a.h"
#include "math/color.h"
#include "assets/asset_system.h"
#include "quake/q_bspfile.h"
#include "stb/stb_image.h"

static u64 ms_version;
static dict_t ms_dict;

static void EnsureInit(void)
{
    if (!ms_dict.keySize)
    {
        dict_new(&ms_dict, sizeof(guid_t), sizeof(textureid_t), EAlloc_Perm);
    }
}

textureid_t texture_load(const char* path)
{
    i32 width = 0;
    i32 height = 0;
    i32 channels = 0;
    u8* texels = stbi_load(path, &width, &height, &channels, 4);
    if (texels)
    {
        texture_t* tex = perm_calloc(sizeof(*tex));
        tex->size = (int2) { width, height };
        tex->texels = (u32*)texels;
        return texture_create(tex);
    }
    return (textureid_t) { 0 };
}

textureid_t texture_create(texture_t* tex)
{
    ASSERT(tex);
    ASSERT(tex->version == 0);
    ASSERT(tex->size.x > 0);
    ASSERT(tex->size.y > 0);
    ASSERT(tex->texels);

    const u64 version = 1099511628211ull + fetch_add_u64(&ms_version, 3, MO_Relaxed);

    tex->version = version;
    textureid_t id = { version, tex };

    return id;
}

bool texture_destroy(textureid_t id)
{
    u64 version = id.version;
    texture_t* tex = id.handle;
    if (tex && cmpex_u64(&(tex->version), &version, version + 1, MO_Release))
    {
        tex->size = (int2) { 0 };
        pim_free(tex->texels);
        tex->texels = NULL;
        pim_free(tex);
        return true;
    }
    return false;
}

bool texture_current(textureid_t id)
{
    texture_t* tex = id.handle;
    return (tex != NULL) && (id.version == load_u64(&(tex->version), MO_Relaxed));
}

bool texture_get(textureid_t id, texture_t* dst)
{
    ASSERT(dst);
    if (texture_current(id))
    {
        *dst = *(texture_t*)id.handle;
        return true;
    }
    return false;
}

bool texture_register(const char* name, textureid_t value)
{
    EnsureInit();

    ASSERT(value.handle);
    guid_t key = StrToGuid(name, Fnv64Bias);
    return dict_add(&ms_dict, &key, &value);
}

textureid_t texture_lookup(const char* name)
{
    EnsureInit();

    guid_t key = StrToGuid(name, Fnv64Bias);
    textureid_t value = { 0 };
    dict_get(&ms_dict, &key, &value);
    return value;
}

static bool ms_paletteLoaded;
static u8 ms_palette[256 * 3];
static void EnsurePalette(void)
{
    if (!ms_paletteLoaded)
    {
        asset_t asset = { 0 };
        if (asset_get("gfx/palette.lmp", &asset))
        {
            ms_paletteLoaded = true;
            const u8* palette = (const u8*)asset.pData;
            ASSERT(asset.length == sizeof(ms_palette));
            memcpy(ms_palette, palette, sizeof(ms_palette));

            // avoid completely black colors, they don't do well with PBR
            for (i32 i = 0; i < sizeof(ms_palette); ++i)
            {
                if (ms_palette[i] == 0)
                {
                    ms_palette[i] = 1;
                }
            }
        }
    }
}

textureid_t texture_unpalette(const u8* bytes, int2 size)
{
    EnsurePalette();

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
            if (true)
            {
                // attempt to turn diffuse map into albedo map
                float4 lin = ColorToLinear(color);
                lin = f4_lerpvs(lin, f4_sqrt(lin), 0.1f);
                texels[i] = LinearToColor(lin);
            }
            else
            {
                texels[i] = color;
            }
        }
    }

    texture_t* tex = perm_calloc(sizeof(*tex));
    tex->size = size;
    tex->texels = texels;
    return texture_create(tex);
}
