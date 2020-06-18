#include "rendering/mesh.h"
#include "allocator/allocator.h"
#include "common/atomics.h"
#include "containers/table.h"
#include "math/float4_funcs.h"
#include <string.h>

static bool ms_once;
static table_t ms_table;

static void EnsureInit(void)
{
    if (!ms_once)
    {
        ms_once = true;
        table_new(&ms_table, sizeof(mesh_t));
    }
}

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

meshid_t mesh_new(mesh_t* mesh, const char* name)
{
    EnsureInit();

    ASSERT(mesh);
    ASSERT(mesh->length > 0);
    ASSERT((mesh->length % 3) == 0);
    ASSERT(mesh->positions);
    ASSERT(mesh->normals);
    ASSERT(mesh->uvs);

    genid id = { -1, 0 };
    if (mesh->length > 0)
    {
        bool added = table_add(&ms_table, name, mesh, &id);
        ASSERT(added);
        if (added)
        {
            mesh_t* meshes = ms_table.values;
            meshes[id.index].bounds = mesh_calcbounds(ToMeshId(id));
        }
    }
    return ToMeshId(id);
}

bool mesh_exists(meshid_t id)
{
    EnsureInit();
    return IsCurrent(id);
}

void mesh_retain(meshid_t id)
{
    EnsureInit();
    table_retain(&ms_table, ToGenId(id));
}

void mesh_release(meshid_t id)
{
    EnsureInit();

    mesh_t mesh = {0};
    if (table_release(&ms_table, ToGenId(id), &mesh))
    {
        pim_free(mesh.positions);
        pim_free(mesh.normals);
        pim_free(mesh.uvs);
    }
}

bool mesh_get(meshid_t id, mesh_t* dst)
{
    ASSERT(dst);
    EnsureInit();
    return table_get(&ms_table, ToGenId(id), dst);
}

bool mesh_set(meshid_t id, mesh_t* src)
{
    ASSERT(src);
    EnsureInit();
    if (IsCurrent(id))
    {
        mesh_t* dst = ms_table.values;
        dst += id.index;
        pim_free(dst->positions);
        pim_free(dst->normals);
        pim_free(dst->uvs);
        memcpy(dst, src, sizeof(*dst));
        memset(src, 0, sizeof(*src));
        return true;
    }
    return false;
}

bool mesh_find(const char* name, meshid_t* idOut)
{
    ASSERT(idOut);
    EnsureInit();
    genid id;
    bool found = table_find(&ms_table, name, &id);
    *idOut = ToMeshId(id);
    return found;
}

sphere_t mesh_calcbounds(meshid_t id)
{
    mesh_t mesh = { 0 };
    if (mesh_get(id, &mesh))
    {
        float4 lo = f4_s(1 << 20);
        float4 hi = f4_s(-(1 << 20));

        const i32 len = mesh.length;
        const float4* pim_noalias positions = mesh.positions;
        for (i32 i = 0; i < len; ++i)
        {
            const float4 pos = positions[i];
            lo = f4_min(lo, pos);
            hi = f4_max(hi, pos);
        }

        float4 center = f4_lerpvs(lo, hi, 0.5f);
        float r = 0.0f;
        for (i32 i = 0; i < len; ++i)
        {
            r = f1_max(r, f4_distancesq3(center, positions[i]));
        }
        center.w = sqrtf(r);
        sphere_t sph = { center };
        return sph;
    }

    sphere_t sph = { 0 };
    return sph;
}
