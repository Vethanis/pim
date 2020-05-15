#include "rendering/mesh.h"
#include "allocator/allocator.h"
#include "common/atomics.h"
#include "containers/dict.h"
#include "common/guid.h"
#include "common/fnv1a.h"
#include "math/float4_funcs.h"

static u64 ms_version;
static dict_t ms_dict;

static void EnsureInit(void)
{
    if (!ms_dict.keySize)
    {
        dict_new(&ms_dict, sizeof(guid_t), sizeof(meshid_t), EAlloc_Perm);
    }
}

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

bool mesh_register(const char* name, meshid_t value)
{
    EnsureInit();

    ASSERT(value.handle);
    guid_t key = StrToGuid(name, Fnv64Bias);
    return dict_add(&ms_dict, &key, &value);
}

meshid_t mesh_lookup(const char* name)
{
    EnsureInit();

    guid_t key = StrToGuid(name, Fnv64Bias);
    meshid_t value = { 0 };
    dict_get(&ms_dict, &key, &value);
    return value;
}

box_t mesh_calcbounds(meshid_t id)
{
    mesh_t mesh = { 0 };
    if (mesh_get(id, &mesh))
    {
        const float kBig = 1 << 20;
        const float kSmall = 1.0f / (1 << 20);

        float4 lo = f4_s(kBig);
        float4 hi = f4_s(kSmall);

        const i32 len = mesh.length;
        const float4* pim_noalias positions = mesh.positions;
        for (i32 i = 0; i < len; ++i)
        {
            const float4 pos = positions[i];
            lo = f4_min(lo, pos);
            hi = f4_max(hi, pos);
        }

        if (len > 0)
        {
            float4 center = f4_lerpvs(lo, hi, 0.5f);
            float4 extents = f4_sub(hi, center);
            box_t box = { center, extents };
            return box;
        }
    }

    box_t box = { f4_0, f4_0 };
    return box;
}
