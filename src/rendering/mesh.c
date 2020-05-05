#include "rendering/mesh.h"
#include "allocator/allocator.h"
#include "common/atomics.h"

static u64 ms_version;

meshid_t mesh_create(mesh_t* mesh)
{
    ASSERT(mesh);
    ASSERT(mesh->version == 0);
    ASSERT(mesh->length > 0);
    ASSERT((mesh->length % 3) == 0);
    ASSERT(mesh->positions);
    ASSERT(mesh->normals);
    ASSERT(mesh->uvs);

    const u64 version = 1099511628211ull + fetch_add_u64(&ms_version, 3, MO_Relaxed);

    mesh->version = version;
    meshid_t id = { version, mesh };

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

