#include "rendering/texture.h"
#include "allocator/allocator.h"
#include "common/atomics.h"
#include "common/random.h"

static prng_t ms_rng;

textureid_t texture_create(texture_t* src)
{
    ASSERT(src);
    ASSERT(src->width > 0);
    ASSERT(src->height > 0);
    ASSERT(src->texels);

    if (!ms_rng.state)
    {
        ms_rng = prng_create();
    }

    const u64 version = prng_u64(&ms_rng);
    texture_t* dst = perm_malloc(sizeof(*dst));
    *dst = *src;
    dst->version = version;
    textureid_t id = { version, dst };

    return id;
}

bool texture_destroy(textureid_t id)
{
    u64 version = id.version;
    texture_t* tex = id.handle;
    if (tex && cmpex_u64(&(tex->version), &version, version + 1, MO_Release))
    {
        tex->width = 0;
        tex->height = 0;
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
