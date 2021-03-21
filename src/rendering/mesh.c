#include "rendering/mesh.h"

#include "allocator/allocator.h"
#include "common/stringutil.h"
#include "common/sort.h"
#include "common/profiler.h"
#include "containers/table.h"
#include "math/float4_funcs.h"
#include "math/float4x4_funcs.h"
#include "math/float3x3_funcs.h"
#include "math/box.h"
#include "ui/cimgui_ext.h"
#include "io/fstr.h"
#include "rendering/vulkan/vkr_mesh.h"
#include "rendering/vulkan/vkr_megamesh.h"
#include "rendering/material.h"
#include "rendering/texture.h"
#include "assets/crate.h"

#include <string.h>

static Table ms_table;

static GenId ToGenId(MeshId mid)
{
    GenId gid;
    gid.index = mid.index;
    gid.version = mid.version;
    return gid;
}

static MeshId ToMeshId(GenId gid)
{
    MeshId mid;
    mid.index = gid.index;
    mid.version = gid.version;
    return mid;
}

static bool IsCurrent(MeshId id)
{
    return table_exists(&ms_table, ToGenId(id));
}

static void FreeMesh(Mesh *const mesh)
{
    vkrMegaMesh_Free(mesh->id);
    Mem_Free(mesh->positions);
    Mem_Free(mesh->normals);
    Mem_Free(mesh->uvs);
    Mem_Free(mesh->texIndices);
    memset(mesh, 0, sizeof(*mesh));
}

void mesh_sys_init(void)
{
    table_new(&ms_table, sizeof(Mesh));
}

void mesh_sys_update(void)
{

}

void mesh_sys_shutdown(void)
{
    Mesh *const pim_noalias meshes = ms_table.values;
    const i32 width = ms_table.width;
    for (i32 i = 0; i < width; ++i)
    {
        if (meshes[i].positions)
        {
            FreeMesh(meshes + i);
        }
    }
    table_del(&ms_table);
}

bool mesh_new(Mesh *const mesh, Guid name, MeshId *const idOut)
{
    ASSERT(mesh);
    ASSERT(mesh->length > 0);
    ASSERT((mesh->length % 3) == 0);
    ASSERT(mesh->positions);
    ASSERT(mesh->normals);
    ASSERT(mesh->uvs);
    ASSERT(mesh->texIndices);

    bool added = false;
    GenId id = { 0, 0 };
    if (mesh->length > 0)
    {
        mesh->id = vkrMegaMesh_Alloc(mesh->length);
        added = table_add(&ms_table, name, mesh, &id);
        if (added)
        {
            added = vkrMegaMesh_Set(
                mesh->id,
                mesh->positions,
                mesh->normals,
                mesh->uvs,
                mesh->texIndices,
                mesh->length);
        }
        ASSERT(added);
    }
    if (!added)
    {
        ASSERT(false);
        FreeMesh(mesh);
    }
    memset(mesh, 0, sizeof(*mesh));
    *idOut = ToMeshId(id);
    return added;
}

bool mesh_exists(MeshId id)
{
    return IsCurrent(id);
}

void mesh_retain(MeshId id)
{
    table_retain(&ms_table, ToGenId(id));
}

void mesh_release(MeshId id)
{
    Mesh mesh = { 0 };
    if (table_release(&ms_table, ToGenId(id), &mesh))
    {
        FreeMesh(&mesh);
    }
}

Mesh *const mesh_get(MeshId id)
{
    return table_get(&ms_table, ToGenId(id));
}

bool mesh_find(Guid name, MeshId *const idOut)
{
    ASSERT(idOut);
    GenId id;
    bool found = table_find(&ms_table, name, &id);
    *idOut = ToMeshId(id);
    return found;
}

bool mesh_getname(MeshId mid, Guid *const dst)
{
    GenId gid = ToGenId(mid);
    return table_getname(&ms_table, gid, dst);
}

Box3D mesh_calcbounds(MeshId id)
{
    Box3D bounds = { 0 };

    Mesh const *const mesh = mesh_get(id);
    if (mesh)
    {
        bounds = box_from_pts(mesh->positions, mesh->length);
    }

    return bounds;
}

bool mesh_setmaterial(MeshId id, Material const *const mat)
{
    Mesh *const mesh = mesh_get(id);
    if (mesh)
    {
        int4 index = { 0 };
        Texture const* tex = texture_get(mat->albedo);
        if (tex)
        {
            index.x = tex->slot.index;
        }
        tex = texture_get(mat->rome);
        if (tex)
        {
            index.y = tex->slot.index;
        }
        tex = texture_get(mat->normal);
        if (tex)
        {
            index.z = tex->slot.index;
        }
        const i32 len = mesh->length;
        int4 *const pim_noalias texIndices = mesh->texIndices;
        for (i32 i = 0; i < len; ++i)
        {
            int4 current = texIndices[i];
            current.x = index.x;
            current.y = index.y;
            current.z = index.z;
            texIndices[i] = current;
        }
        return mesh_update(mesh);
    }
    return false;
}

bool VEC_CALL mesh_settransform(MeshId id, float4x4 localToWorld)
{
    Mesh *const mesh = mesh_get(id);
    if (mesh)
    {
        float3x3 IM = f3x3_IM(localToWorld);
        float4 *const pim_noalias positions = mesh->positions;
        float4 *const pim_noalias normals = mesh->normals;
        const i32 len = mesh->length;
        for (i32 i = 0; i < len; ++i)
        {
            positions[i] = f4x4_mul_pt(localToWorld, positions[i]);
        }
        for (i32 i = 0; i < len; ++i)
        {
            normals[i] = f4_normalize3(f3x3_mul_col(IM, normals[i]));
        }
        return mesh_update(mesh);
    }
    return false;
}

bool mesh_update(Mesh *const mesh)
{
    ASSERT(mesh);

    if (vkrMegaMesh_Set(
        mesh->id,
        mesh->positions,
        mesh->normals,
        mesh->uvs,
        mesh->texIndices,
        mesh->length))
    {
        // fast path: no length change
        return true;
    }

    // slow path, have to free and reallocate.
    vkrMegaMesh_Free(mesh->id);
    if (mesh->length > 0)
    {
        mesh->id = vkrMegaMesh_Alloc(mesh->length);
        return vkrMegaMesh_Set(
            mesh->id,
            mesh->positions,
            mesh->normals,
            mesh->uvs,
            mesh->texIndices,
            mesh->length);
    }

    // empty mesh, no point in holding an id for this mesh.
    return false;
}

bool mesh_save(Crate *const crate, MeshId id, Guid *const dst)
{
    if (mesh_getname(id, dst))
    {
        const Mesh *const meshes = ms_table.values;
        const Mesh mesh = meshes[id.index];
        const i32 len = mesh.length;
        const i32 hdrBytes = sizeof(DiskMesh);
        const i32 positionBytes = sizeof(mesh.positions[0]) * len;
        const i32 normalBytes = sizeof(mesh.normals[0]) * len;
        const i32 uvBytes = sizeof(mesh.uvs[0]) * len;
        const i32 texIndBytes = sizeof(mesh.texIndices[0]) * len;
        const i32 totalBytes = hdrBytes + positionBytes + normalBytes + uvBytes + texIndBytes;
        DiskMesh* dmesh = Perm_Alloc(totalBytes);
        dmesh->version = kMeshVersion;
        dmesh->length = len;
        guid_get_name(*dst, ARGS(dmesh->name));
        float4* positions = (float4*)(dmesh + 1);
        float4* normals = positions + len;
        float4* uvs = normals + len;
        int4* texIndices = (int4*)(uvs + len);
        memcpy(positions, mesh.positions, positionBytes);
        memcpy(normals, mesh.normals, normalBytes);
        memcpy(uvs, mesh.uvs, uvBytes);
        memcpy(texIndices, mesh.texIndices, texIndBytes);
        bool wasSet = crate_set(crate, *dst, dmesh, totalBytes);
        Mem_Free(dmesh);
        return wasSet;
    }
    return false;
}

bool mesh_load(Crate *const crate, Guid name, MeshId *const dst)
{
    if (mesh_find(name, dst))
    {
        mesh_retain(*dst);
        return true;
    }

    bool loaded = false;
    Mesh mesh = { 0 };
    i32 offset = 0;
    i32 size = 0;
    if (crate_stat(crate, name, &offset, &size))
    {
        DiskMesh* dmesh = Perm_Alloc(size);
        if (dmesh && crate_get(crate, name, dmesh, size))
        {
            if (dmesh->version == kMeshVersion)
            {
                const i32 len = dmesh->length;
                if (len > 0)
                {
                    guid_set_name(name, dmesh->name);
                    mesh.length = len;
                    mesh.positions = Perm_Alloc(sizeof(mesh.positions[0]) * mesh.length);
                    mesh.normals = Perm_Alloc(sizeof(mesh.normals[0]) * mesh.length);
                    mesh.uvs = Perm_Alloc(sizeof(mesh.uvs[0]) * mesh.length);
                    mesh.texIndices = Perm_Alloc(sizeof(mesh.texIndices[0]) * mesh.length);

                    float4* positions = (float4*)(dmesh + 1);
                    float4* normals = positions + len;
                    float4* uvs = normals + len;
                    int4* texIndices = (int4*)(uvs + len);
                    memcpy(mesh.positions, positions, sizeof(mesh.positions[0]) * len);
                    memcpy(mesh.normals, normals, sizeof(mesh.normals[0]) * len);
                    memcpy(mesh.uvs, uvs, sizeof(mesh.uvs[0]) * len);
                    memcpy(mesh.texIndices, texIndices, sizeof(mesh.texIndices[0]) * len);
                    loaded = mesh_new(&mesh, name, dst);
                }
            }
        }
        Mem_Free(dmesh);
    }
    return loaded;
}

// ----------------------------------------------------------------------------

static char gs_search[PIM_PATH];
static i32 gs_selection;
static i32 gs_cmpmode;
static bool gs_revsort;

static i32 CmpSlotFn(i32 ilhs, i32 irhs, void* usr)
{
    const Guid* pim_noalias names = ms_table.names;
    const Mesh* pim_noalias meshes = ms_table.values;
    const i32* pim_noalias refcounts = ms_table.refcounts;
    const i32 mode = gs_cmpmode;

    Guid lhs = names[ilhs];
    Guid rhs = names[irhs];
    bool lvalid = !guid_isnull(lhs);
    bool rvalid = !guid_isnull(rhs);
    if (lvalid && rvalid)
    {
        i32 cmp = 0;
        switch (mode)
        {
        default:
        case 0:
        {
            char lname[64] = { 0 };
            char rname[64] = { 0 };
            guid_get_name(lhs, ARGS(lname));
            guid_get_name(rhs, ARGS(rname));
            cmp = StrCmp(ARGS(lname), rname);
        }
        break;
        case 1:
            cmp = meshes[ilhs].length - meshes[irhs].length;
            break;
        case 2:
            cmp = refcounts[ilhs] - refcounts[irhs];
            break;
        }
        return gs_revsort ? -cmp : cmp;
    }
    if (lvalid)
    {
        return -1;
    }
    if (rvalid)
    {
        return 1;
    }
    return 0;
}

ProfileMark(pm_OnGui, mesh_sys_gui)
void mesh_sys_gui(bool* pEnabled)
{
    ProfileBegin(pm_OnGui);

    i32 selection = gs_selection;
    if (igBegin("Meshes", pEnabled, 0))
    {
        igInputText("Search", ARGS(gs_search), 0x0, NULL, NULL);

        const i32 width = ms_table.width;
        const Guid* names = ms_table.names;
        const Mesh* meshes = ms_table.values;
        const i32* refcounts = ms_table.refcounts;

        i32 bytesUsed = 0;
        for (i32 i = 0; i < width; ++i)
        {
            if (!guid_isnull(names[i]))
            {
                if (gs_search[0])
                {
                    char name[64] = { 0 };
                    guid_get_name(names[i], ARGS(name));
                    if (!StrIStr(ARGS(name), gs_search))
                    {
                        continue;
                    }
                }
                i32 length = meshes[i].length;
                bytesUsed += sizeof(meshes[0]);
                bytesUsed += length * sizeof(meshes[0].positions[0]);
                bytesUsed += length * sizeof(meshes[0].normals[0]);
                bytesUsed += length * sizeof(meshes[0].uvs[0]);
                bytesUsed += length * sizeof(meshes[0].texIndices[0]);
            }
        }

        igText("Bytes Used: %d", bytesUsed);

        if (igExButton("Clear Selection"))
        {
            selection = -1;
        }

        igSeparator();

        const char* const titles[] =
        {
            "Name",
            "Length",
            "References",
            "Select",
        };
        if (igExTableHeader(NELEM(titles), titles, &gs_cmpmode))
        {
            gs_revsort = !gs_revsort;
        }

        i32* indices = Temp_Calloc(sizeof(indices[0]) * width);
        for (i32 i = 0; i < width; ++i)
        {
            indices[i] = i;
        }
        sort_i32(indices, width, CmpSlotFn, NULL);

        for (i32 i = 0; i < width; ++i)
        {
            i32 j = indices[i];
            ASSERT(j >= 0);
            ASSERT(j < width);
            Guid name = names[j];
            if (guid_isnull(name))
            {
                continue;
            }

            if (gs_search[0])
            {
                char namestr[64] = { 0 };
                guid_get_name(name, ARGS(namestr));
                if (!StrIStr(ARGS(namestr), gs_search))
                {
                    continue;
                }
            }

            i32 length = meshes[j].length;
            i32 refcount = refcounts[j];
            char namestr[PIM_PATH] = { 0 };
            if (!guid_get_name(name, ARGS(namestr)))
            {
                guid_fmt(ARGS(namestr), name);
            }
            igText(namestr); igNextColumn();
            igText("%d", length); igNextColumn();
            igText("%d", refcount); igNextColumn();
            const char* selectText = selection == j ? "Selected" : "Select";
            igPushIDInt(j);
            if (igExButton(selectText))
            {
                selection = j;
            }
            igPopID();
            igNextColumn();
        }

        igExTableFooter();
    }
    igEnd();
    ProfileEnd(pm_OnGui);
}
