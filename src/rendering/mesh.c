#include "rendering/mesh.h"
#include "allocator/allocator.h"
#include "common/atomics.h"
#include "containers/table.h"
#include "math/float4_funcs.h"
#include "math/box.h"
#include <string.h>

static table_t ms_table;

static genid ToGenId(meshid_t id)
{
    return (genid) { .index = id.index, .version = id.version };
}

static meshid_t ToMeshId(genid id)
{
    return (meshid_t) { .index = id.index, .version = id.version };
}

static bool IsCurrent(meshid_t id)
{
    return table_exists(&ms_table, ToGenId(id));
}

static void FreeMesh(mesh_t* mesh)
{
    pim_free(mesh->positions);
    pim_free(mesh->normals);
    pim_free(mesh->uvs);
    memset(mesh, 0, sizeof(*mesh));
}

void mesh_sys_init(void)
{
    table_new(&ms_table, sizeof(mesh_t));
}

void mesh_sys_update(void)
{

}

void mesh_sys_shutdown(void)
{
    const char** names = ms_table.names;
    mesh_t* meshes = ms_table.values;
    i32 width = ms_table.width;
    for (i32 i = 0; i < width; ++i)
    {
        if (names[i])
        {
            FreeMesh(meshes + i);
        }
    }
    table_del(&ms_table);
}

bool mesh_new(mesh_t* mesh, const char* name, meshid_t* idOut)
{
    ASSERT(mesh);
    ASSERT(mesh->length > 0);
    ASSERT((mesh->length % 3) == 0);
    ASSERT(mesh->positions);
    ASSERT(mesh->normals);
    ASSERT(mesh->uvs);

    bool added = false;
    genid id = { 0, 0 };
    if (mesh->length > 0)
    {
        added = table_add(&ms_table, name, mesh, &id);
        ASSERT(added);
        if (added)
        {
            mesh_calcbounds(ToMeshId(id));
        }
    }
    if (!added)
    {
        FreeMesh(mesh);
    }
    *idOut = ToMeshId(id);
    return added;
}

bool mesh_exists(meshid_t id)
{
    return IsCurrent(id);
}

void mesh_retain(meshid_t id)
{
    table_retain(&ms_table, ToGenId(id));
}

void mesh_release(meshid_t id)
{
    mesh_t mesh = { 0 };
    if (table_release(&ms_table, ToGenId(id), &mesh))
    {
        FreeMesh(&mesh);
    }
}

bool mesh_get(meshid_t id, mesh_t* dst)
{
    ASSERT(dst);
    return table_get(&ms_table, ToGenId(id), dst);
}

bool mesh_set(meshid_t id, mesh_t* src)
{
    ASSERT(src);
    if (IsCurrent(id))
    {
        mesh_t* dst = ms_table.values;
        dst += id.index;
        if (dst->positions != src->positions)
        {
            FreeMesh(dst);
        }
        memcpy(dst, src, sizeof(*dst));
        memset(src, 0, sizeof(*src));
        return true;
    }
    return false;
}

bool mesh_find(const char* name, meshid_t* idOut)
{
    ASSERT(idOut);
    genid id;
    bool found = table_find(&ms_table, name, &id);
    *idOut = ToMeshId(id);
    return found;
}

box_t mesh_calcbounds(meshid_t id)
{
    box_t bounds = { 0 };

    mesh_t mesh;
    if (mesh_get(id, &mesh))
    {
        bounds = box_from_pts(mesh.positions, mesh.length);
        mesh_t* dst = ms_table.values;
        dst += id.index;
        dst->bounds = bounds;
    }

    return bounds;
}
