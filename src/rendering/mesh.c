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
        mesh_t* dst = ms_table.values;
        dst += id.index;
        if (memcmp(dst, src, sizeof(*dst)))
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

// ----------------------------------------------------------------------------

static char gs_search[PIM_PATH];
static i32 gs_selection;
static i32 gs_cmpmode;
static bool gs_revsort;

static i32 CmpSlotFn(i32 ilhs, i32 irhs, void* usr)
{
    const i32 width = ms_table.width;
    const char** names = ms_table.names;
    const mesh_t* meshes = ms_table.values;
    const i32* refcounts = ms_table.refcounts;

    if (names[ilhs] && names[irhs])
    {
        i32 cmp = 0;
        switch (gs_cmpmode)
        {
        default:
        case 0:
            cmp = StrCmp(names[ilhs], PIM_PATH, names[irhs]);
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
    if (names[ilhs])
    {
        return -1;
    }
    if (names[irhs])
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
        igInputText("Search", gs_search, NELEM(gs_search), 0, 0, 0);
        const char* search = gs_search;

        const i32 width = ms_table.width;
        const char** names = ms_table.names;
        const mesh_t* meshes = ms_table.values;
        const i32* refcounts = ms_table.refcounts;

        i32 bytesUsed = 0;
        for (i32 i = 0; i < width; ++i)
        {
            const char* name = names[i];
            if (name)
            {
                if (search[0] && !StrIStr(name, PIM_PATH, search))
                {
                    continue;
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
            const char* name = names[j];
            if (!name)
            {
                continue;
            }
            if (search[0] && !StrIStr(name, PIM_PATH, search))
            {
                continue;
            }

            i32 length = meshes[j].length;
            i32 refcount = refcounts[j];
            igText(name); igNextColumn();
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
