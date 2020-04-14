#include "rendering/mesh.h"
#include "allocator/allocator.h"
#include "common/atomics.h"
#include "common/random.h"

static prng_t ms_rng;

meshid_t mesh_create(mesh_t* src)
{
    ASSERT(src);
    ASSERT(src->length > 0);
    ASSERT((src->length % 3) == 0);
    ASSERT(src->positions);
    ASSERT(src->normals);
    ASSERT(src->uvs);

    if (!ms_rng.state)
    {
        ms_rng = prng_create();
    }

    const u64 version = prng_u64(&ms_rng);
    mesh_t* dst = perm_malloc(sizeof(*dst));
    *dst = *src;
    dst->version = version;
    meshid_t id = { version, dst };

    return id;
}

bool mesh_destroy(meshid_t id)
{
    u64 version = id.version;
    mesh_t* mesh = id.handle;
    if (mesh && cmpex_u64(&(mesh->version), &version, version + 1, MO_Release))
    {
        mesh->length = 0;
        pim_free(mesh->positions);
        mesh->positions = NULL;
        pim_free(mesh->normals);
        mesh->normals = NULL;
        pim_free(mesh->uvs);
        mesh->uvs = NULL;
        pim_free(mesh);
        return true;
    }
    return false;
}

bool mesh_current(meshid_t id)
{
    mesh_t* mesh = id.handle;
    return (mesh != NULL) && (id.version == load_u64(&(mesh->version), MO_Relaxed));
}

bool mesh_get(meshid_t id, mesh_t* dst)
{
    ASSERT(dst);
    if (mesh_current(id))
    {
        *dst = *(mesh_t*)id.handle;
        return true;
    }
    return false;
}

