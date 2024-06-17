#include "rendering/mesh.h"

#include "allocator/allocator.h"
#include "common/stringutil.h"
#include "common/sort.h"
#include "common/profiler.h"
#include "common/console.h"
#include "containers/table.h"
#include "math/float4_funcs.h"
#include "math/float4x4_funcs.h"
#include "math/float3x3_funcs.h"
#include "math/box.h"
#include "ui/cimgui_ext.h"
#include "io/fstr.h"
#include "rendering/vulkan/vkr_mesh.h"
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
    return Table_Exists(&ms_table, ToGenId(id));
}

static void FreeMesh(Mesh *const mesh)
{
    vkrMesh_Del(mesh->id);
    Mem_Free(mesh->positions);
    Mem_Free(mesh->normals);
    Mem_Free(mesh->tangents);
    Mem_Free(mesh->uvs);
    Mem_Free(mesh->texIndices);
    memset(mesh, 0, sizeof(*mesh));
}

void MeshSys_Init(void)
{
    Table_New(&ms_table, sizeof(Mesh));
}

void MeshSys_Update(void)
{

}

void MeshSys_Shutdown(void)
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
    Table_Del(&ms_table);
}

bool Mesh_New(Mesh *const mesh, Guid name, MeshId *const idOut)
{
    ASSERT(mesh);
    ASSERT(mesh->length > 0);
    ASSERT((mesh->length % 3) == 0);
    ASSERT(mesh->positions);
    ASSERT(mesh->normals);
    ASSERT(mesh->tangents);
    ASSERT(mesh->uvs);
    ASSERT(mesh->texIndices);

    bool added = false;
    GenId id = { 0, 0 };
    if (mesh->length > 0)
    {
        added = Table_Add(&ms_table, name, mesh, &id);
        if (added)
        {
            vkrMeshId vkId = vkrMesh_New(mesh->length, mesh->positions, mesh->normals, mesh->tangents, mesh->uvs, mesh->texIndices);
            Mesh* pim_noalias meshes = ms_table.values;
            meshes[id.index].id = vkId;
            if (!vkrMesh_Exists(vkId))
            {
                char nameStr[PIM_PATH];
                Guid_GetName(name, ARGS(nameStr));
                Con_Logf(LogSev_Error, "mesh", "Failed to create vkrMesh for mesh named '%s'", nameStr);
                ASSERT(false);
                added = false;
                Table_Release(&ms_table, id, NULL);
            }
        }
    }
    if (!added)
    {
        FreeMesh(mesh);
    }
    memset(mesh, 0, sizeof(*mesh));
    *idOut = ToMeshId(id);
    return added;
}

bool Mesh_Exists(MeshId id)
{
    return IsCurrent(id);
}

void Mesh_Retain(MeshId id)
{
    Table_Retain(&ms_table, ToGenId(id));
}

void Mesh_Release(MeshId id)
{
    Mesh mesh = { 0 };
    if (Table_Release(&ms_table, ToGenId(id), &mesh))
    {
        FreeMesh(&mesh);
    }
}

Mesh *const Mesh_Get(MeshId id)
{
    return Table_Get(&ms_table, ToGenId(id));
}

bool Mesh_Find(Guid name, MeshId *const idOut)
{
    ASSERT(idOut);
    GenId id;
    bool found = Table_Find(&ms_table, name, &id);
    *idOut = ToMeshId(id);
    return found;
}

bool Mesh_GetName(MeshId mid, Guid *const dst)
{
    GenId gid = ToGenId(mid);
    return Table_GetName(&ms_table, gid, dst);
}

Box3D Mesh_CalcBounds(MeshId id)
{
    Box3D bounds = { f4_0, f4_0 };
    Mesh const *const mesh = Mesh_Get(id);
    if (mesh)
    {
        bounds = box_from_pts(mesh->positions, mesh->length);
    }
    return bounds;
}

bool Mesh_Upload(MeshId id)
{
    Mesh* mesh = Mesh_Get(id);
    if (mesh)
    {
        return vkrMesh_Upload(mesh->id, mesh->length, mesh->positions, mesh->normals, mesh->tangents, mesh->uvs, mesh->texIndices);
    }
    return false;
}

bool Mesh_Save(Crate *const crate, MeshId id, Guid *const dst)
{
    if (Mesh_GetName(id, dst))
    {
        const Mesh *const meshes = ms_table.values;
        const Mesh mesh = meshes[id.index];
        const i32 len = mesh.length;
        const i32 hdrBytes = sizeof(DiskMesh);
        const i32 positionBytes = sizeof(mesh.positions[0]) * len;
        const i32 normalBytes = sizeof(mesh.normals[0]) * len;
        const i32 tangentBytes = sizeof(mesh.tangents[0]) * len;
        const i32 uvBytes = sizeof(mesh.uvs[0]) * len;
        const i32 texIndBytes = sizeof(mesh.texIndices[0]) * len;
        const i32 totalBytes = hdrBytes + positionBytes + normalBytes + tangentBytes + uvBytes + texIndBytes;
        DiskMesh* dmesh = Perm_Alloc(totalBytes);
        dmesh->version = kMeshVersion;
        dmesh->length = len;
        Guid_GetName(*dst, ARGS(dmesh->name));
        float4* positions = (float4*)(dmesh + 1);
        float4* normals = positions + len;
        float4* tangents = normals + len;
        float4* uvs = tangents + len;
        int4* texIndices = (int4*)(uvs + len);
        memcpy(positions, mesh.positions, positionBytes);
        memcpy(normals, mesh.normals, normalBytes);
        memcpy(tangents, mesh.tangents, tangentBytes);
        memcpy(uvs, mesh.uvs, uvBytes);
        memcpy(texIndices, mesh.texIndices, texIndBytes);
        bool wasSet = Crate_Set(crate, *dst, dmesh, totalBytes);
        Mem_Free(dmesh);
        return wasSet;
    }
    return false;
}

bool Mesh_Load(Crate *const crate, Guid name, MeshId *const dst)
{
    if (Mesh_Find(name, dst))
    {
        Mesh_Retain(*dst);
        return true;
    }

    bool loaded = false;
    Mesh mesh = { 0 };
    i32 offset = 0;
    i32 size = 0;
    if (Crate_Stat(crate, name, &offset, &size))
    {
        DiskMesh* dmesh = Perm_Alloc(size);
        if (dmesh && Crate_Get(crate, name, dmesh, size))
        {
            if (dmesh->version == kMeshVersion)
            {
                const i32 len = dmesh->length;
                if (len > 0)
                {
                    Guid_SetName(name, dmesh->name);
                    mesh.length = len;
                    mesh.positions = Perm_Alloc(sizeof(mesh.positions[0]) * mesh.length);
                    mesh.normals = Perm_Alloc(sizeof(mesh.normals[0]) * mesh.length);
                    mesh.tangents = Perm_Alloc(sizeof(mesh.tangents[0]) * mesh.length);
                    mesh.uvs = Perm_Alloc(sizeof(mesh.uvs[0]) * mesh.length);
                    mesh.texIndices = Perm_Alloc(sizeof(mesh.texIndices[0]) * mesh.length);

                    float4* positions = (float4*)(dmesh + 1);
                    float4* normals = positions + len;
                    float4* tangents = normals + len;
                    float4* uvs = tangents + len;
                    int4* texIndices = (int4*)(uvs + len);
                    memcpy(mesh.positions, positions, sizeof(mesh.positions[0]) * len);
                    memcpy(mesh.normals, normals, sizeof(mesh.normals[0]) * len);
                    memcpy(mesh.tangents, tangents, sizeof(mesh.tangents[0]) * len);
                    memcpy(mesh.uvs, uvs, sizeof(mesh.uvs[0]) * len);
                    memcpy(mesh.texIndices, texIndices, sizeof(mesh.texIndices[0]) * len);
                    loaded = Mesh_New(&mesh, name, dst);
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
    bool lvalid = !Guid_IsNull(lhs);
    bool rvalid = !Guid_IsNull(rhs);
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
            Guid_GetName(lhs, ARGS(lname));
            Guid_GetName(rhs, ARGS(rname));
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

ProfileMark(pm_OnGui, MeshSys_Gui)
void MeshSys_Gui(bool* pEnabled)
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
            if (!Guid_IsNull(names[i]))
            {
                if (gs_search[0])
                {
                    char name[64] = { 0 };
                    Guid_GetName(names[i], ARGS(name));
                    if (!StrIStr(ARGS(name), gs_search))
                    {
                        continue;
                    }
                }
                i32 length = meshes[i].length;
                bytesUsed += sizeof(meshes[0]);
                bytesUsed += length * sizeof(meshes[0].positions[0]);
                bytesUsed += length * sizeof(meshes[0].normals[0]);
                bytesUsed += length * sizeof(meshes[0].tangents[0]);
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
        QuickSort_Int(indices, width, CmpSlotFn, NULL);

        for (i32 i = 0; i < width; ++i)
        {
            i32 j = indices[i];
            ASSERT(j >= 0);
            ASSERT(j < width);
            Guid name = names[j];
            if (Guid_IsNull(name))
            {
                continue;
            }

            if (gs_search[0])
            {
                char namestr[64] = { 0 };
                Guid_GetName(name, ARGS(namestr));
                if (!StrIStr(ARGS(namestr), gs_search))
                {
                    continue;
                }
            }

            i32 length = meshes[j].length;
            i32 refcount = refcounts[j];
            char namestr[PIM_PATH] = { 0 };
            if (!Guid_GetName(name, ARGS(namestr)))
            {
                Guid_Format(ARGS(namestr), name);
            }
            igText(namestr); igNextColumn();
            igText("%d", length); igNextColumn();
            igText("%d", refcount); igNextColumn();
            const char* selectText = selection == j ? "Selected" : "Select";
            igPushID_Int(j);
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
