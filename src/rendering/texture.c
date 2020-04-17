#include "rendering/texture.h"
#include "allocator/allocator.h"
#include "common/atomics.h"

static u64 ms_version;

textureid_t texture_create(texture_t* src)
{
    ASSERT(src);
    ASSERT(src->width > 0);
    ASSERT(src->height > 0);
    ASSERT(src->texels);

    const u64 version = 1099511628211ull + fetch_add_u64(&ms_version, 3, MO_Relaxed);

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
