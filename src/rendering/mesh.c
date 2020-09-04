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
    vkrMesh_Del(&mesh->vkrmesh);
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
    const guid_t* pim_noalias names = ms_table.names;
    mesh_t* pim_noalias meshes = ms_table.values;
    const i32 width = ms_table.width;
    for (i32 i = 0; i < width; ++i)
    {
        if (!guid_isnull(names[i]))
        {
            FreeMesh(meshes + i);
        }
    }
    table_del(&ms_table);
}

void mesh_sys_vkfree(void)
{
    const guid_t* pim_noalias names = ms_table.names;
    mesh_t* pim_noalias meshes = ms_table.values;
    const i32 width = ms_table.width;
    for (i32 i = 0; i < width; ++i)
    {
        if (!guid_isnull(names[i]))
        {
            vkrMesh_Del(&meshes[i].vkrmesh);
        }
    }
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
        added = vkrMesh_New(&mesh->vkrmesh, mesh->length, mesh->positions, mesh->normals, mesh->uvs, 0, NULL);
        ASSERT(added);
        if (added)
        {
            added = table_add(&ms_table, name, mesh, &id);
        }
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
        mesh_t* meshes = ms_table.values;
        i32 index = id.index;
        memcpy(&meshes[index], src, sizeof(meshes[0]));
        memset(src, 0, sizeof(*src));
        return true;
    }
    return false;
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

    mesh_t mesh;
    if (mesh_get(id, &mesh))
    {
        bounds = box_from_pts(mesh.positions, mesh.length);
    }

    return bounds;
}

bool mesh_save(meshid_t id, guid_t* dst)
{
    if (mesh_getname(id, dst))
    {
        char filename[PIM_PATH] = "data/";
        guid_tofile(ARGS(filename), *dst, ".mesh");
        fstr_t fd = fstr_open(filename, "wb");
        if (fstr_isopen(fd))
        {
            const mesh_t* meshes = ms_table.values;
            const mesh_t mesh = meshes[id.index];
            i32 offset = 0;
            dmesh_t dmesh = { 0 };
            dbytes_new(1, sizeof(dmesh), &offset);
            dmesh.version = kMeshVersion;
            dmesh.length = mesh.length;
            dmesh.positions = dbytes_new(mesh.length, sizeof(mesh.positions[0]), &offset);
            dmesh.normals = dbytes_new(mesh.length, sizeof(mesh.normals[0]), &offset);
            dmesh.uvs = dbytes_new(mesh.length, sizeof(mesh.uvs[0]), &offset);
            fstr_write(fd, &dmesh, sizeof(dmesh));

            ASSERT(fstr_tell(fd) == dmesh.positions.offset);
            fstr_write(fd, mesh.positions, sizeof(mesh.positions[0]) * mesh.length);
            ASSERT(fstr_tell(fd) == dmesh.normals.offset);
            fstr_write(fd, mesh.normals, sizeof(mesh.normals[0]) * mesh.length);
            ASSERT(fstr_tell(fd) == dmesh.uvs.offset);
            fstr_write(fd, mesh.uvs, sizeof(mesh.uvs[0]) * mesh.length);

            fstr_close(&fd);
            return true;
        }
    }
    return false;
}

bool mesh_load(guid_t name, meshid_t* dst)
{
    bool loaded = false;

    char filename[PIM_PATH] = "data/";
    guid_tofile(ARGS(filename), name, ".mesh");
    fstr_t fd = fstr_open(filename, "rb");
    mesh_t mesh = { 0 };
    if (fstr_isopen(fd))
    {
        dmesh_t dmesh = { 0 };
        fstr_read(fd, &dmesh, sizeof(dmesh));
        if (dmesh.version == kMeshVersion)
        {
            dbytes_check(dmesh.positions, sizeof(mesh.positions[0]));
            dbytes_check(dmesh.normals, sizeof(mesh.normals[0]));
            dbytes_check(dmesh.uvs, sizeof(mesh.uvs[0]));

            mesh.length = dmesh.length;
            if (mesh.length > 0)
            {
                mesh.positions = perm_malloc(sizeof(mesh.positions[0]) * mesh.length);
                mesh.normals = perm_malloc(sizeof(mesh.normals[0]) * mesh.length);
                mesh.uvs = perm_malloc(sizeof(mesh.uvs[0]) * mesh.length);

                ASSERT(fstr_tell(fd) == dmesh.positions.offset);
                fstr_read(fd, mesh.positions, sizeof(mesh.positions[0]) * mesh.length);
                ASSERT(fstr_tell(fd) == dmesh.normals.offset);
                fstr_read(fd, mesh.normals, sizeof(mesh.normals[0]) * mesh.length);
                ASSERT(fstr_tell(fd) == dmesh.uvs.offset);
                fstr_read(fd, mesh.uvs, sizeof(mesh.uvs[0]) * mesh.length);

                loaded = mesh_new(&mesh, name, dst);
            }
        }
    }
cleanup:
    fstr_close(&fd);
    if (!loaded)
    {
        FreeMesh(&mesh);
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
            cmp = guid_cmp(lhs, rhs);
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
        const i32 width = ms_table.width;
        const guid_t* names = ms_table.names;
        const mesh_t* meshes = ms_table.values;
        const i32* refcounts = ms_table.refcounts;

        i32 bytesUsed = 0;
        for (i32 i = 0; i < width; ++i)
        {
            if (!guid_isnull(names[i]))
            {
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

            i32 length = meshes[j].length;
            i32 refcount = refcounts[j];
            char namestr[PIM_PATH] = { 0 };
            guid_fmt(ARGS(namestr), name);
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
