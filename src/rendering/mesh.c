#include "rendering/mesh.h"

#include "allocator/allocator.h"
#include "common/stringutil.h"
#include "common/sort.h"
#include "common/profiler.h"
#include "containers/table.h"
#include "math/float4_funcs.h"
#include "math/box.h"
#include "ui/cimgui.h"
#include "ui/cimgui_ext.h"
#include "io/fstr.h"
#include "rendering/vulkan/vkr_mesh.h"
#include "assets/crate.h"

#include <string.h>

static table_t ms_table;

pim_inline genid ToGenId(meshid_t mid)
{
    genid gid;
    gid.index = mid.index;
    gid.version = mid.version;
    return gid;
}

pim_inline meshid_t ToMeshId(genid gid)
{
    meshid_t mid;
    mid.index = gid.index;
    mid.version = gid.version;
    return mid;
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
    mesh_t* pim_noalias meshes = ms_table.values;
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

bool mesh_new(mesh_t* mesh, guid_t name, meshid_t* idOut)
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
    }
    if (!added)
    {
        ASSERT(false);
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

mesh_t* mesh_get(meshid_t id)
{
    return table_get(&ms_table, ToGenId(id));
}

bool mesh_find(guid_t name, meshid_t* idOut)
{
    ASSERT(idOut);
    genid id;
    bool found = table_find(&ms_table, name, &id);
    *idOut = ToMeshId(id);
    return found;
}

bool mesh_getname(meshid_t mid, guid_t* dst)
{
    genid gid = ToGenId(mid);
    return table_getname(&ms_table, gid, dst);
}

box_t mesh_calcbounds(meshid_t id)
{
    box_t bounds = { 0 };

    mesh_t const *const mesh = mesh_get(id);
    if (mesh)
    {
        bounds = box_from_pts(mesh->positions, mesh->length);
    }

    return bounds;
}

bool mesh_save(crate_t* crate, meshid_t id, guid_t* dst)
{
    if (mesh_getname(id, dst))
    {
        const mesh_t* meshes = ms_table.values;
        const mesh_t mesh = meshes[id.index];
        const i32 len = mesh.length;
        const i32 hdrBytes = sizeof(dmesh_t);
        const i32 positionBytes = sizeof(mesh.positions[0]) * len;
        const i32 normalBytes = sizeof(mesh.normals[0]) * len;
        const i32 uvBytes = sizeof(mesh.uvs[0]) * len;
        const i32 totalBytes = hdrBytes + positionBytes + normalBytes + uvBytes;
        dmesh_t* dmesh = perm_malloc(totalBytes);
        dmesh->version = kMeshVersion;
        dmesh->length = len;
        guid_get_name(*dst, ARGS(dmesh->name));
        float4* positions = (float4*)(dmesh + 1);
        float4* normals = positions + len;
        float4* uvs = normals + len;
        memcpy(positions, mesh.positions, positionBytes);
        memcpy(normals, mesh.normals, normalBytes);
        memcpy(uvs, mesh.uvs, uvBytes);
        bool wasSet = crate_set(crate, *dst, dmesh, totalBytes);
        pim_free(dmesh);
        return wasSet;
    }
    return false;
}

bool mesh_load(crate_t* crate, guid_t name, meshid_t* dst)
{
    bool loaded = false;
    mesh_t mesh = { 0 };
    i32 offset = 0;
    i32 size = 0;
    if (crate_stat(crate, name, &offset, &size))
    {
        dmesh_t* dmesh = perm_malloc(size);
        if (dmesh && crate_get(crate, name, dmesh, size))
        {
            if (dmesh->version == kMeshVersion)
            {
                const i32 len = dmesh->length;
                if (len > 0)
                {
                    guid_set_name(name, dmesh->name);
                    mesh.length = len;
                    mesh.positions = perm_malloc(sizeof(mesh.positions[0]) * mesh.length);
                    mesh.normals = perm_malloc(sizeof(mesh.normals[0]) * mesh.length);
                    mesh.uvs = perm_malloc(sizeof(mesh.uvs[0]) * mesh.length);

                    float4* positions = (float4*)(dmesh + 1);
                    float4* normals = positions + len;
                    float4* uvs = normals + len;
                    memcpy(mesh.positions, positions, sizeof(mesh.positions[0]) * len);
                    memcpy(mesh.normals, normals, sizeof(mesh.normals[0]) * len);
                    memcpy(mesh.uvs, uvs, sizeof(mesh.uvs[0]) * len);
                    loaded = mesh_new(&mesh, name, dst);
                }
            }
        }
        pim_free(dmesh);
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
    const guid_t* pim_noalias names = ms_table.names;
    const mesh_t* pim_noalias meshes = ms_table.values;
    const i32* pim_noalias refcounts = ms_table.refcounts;
    const i32 mode = gs_cmpmode;

    guid_t lhs = names[ilhs];
    guid_t rhs = names[irhs];
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
        const guid_t* names = ms_table.names;
        const mesh_t* meshes = ms_table.values;
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
            }
        }

        igText("Bytes Used: %d", bytesUsed);

        if (igButton("Clear Selection"))
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
        if (igTableHeader(NELEM(titles), titles, &gs_cmpmode))
        {
            gs_revsort = !gs_revsort;
        }

        i32* indices = tmp_calloc(sizeof(indices[0]) * width);
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
            guid_t name = names[j];
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
            if (igButton(selectText))
            {
                selection = j;
            }
            igPopID();
            igNextColumn();
        }

        igTableFooter();
    }
    igEnd();
    ProfileEnd(pm_OnGui);
}
