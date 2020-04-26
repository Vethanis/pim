#include "assets/asset_system.h"

#include "allocator/allocator.h"
#include "common/cvar.h"
#include "common/hashstring.h"
#include "common/profiler.h"
#include "common/sort.h"
#include "common/stringutil.h"
#include "containers/dict.h"
#include "quake/packfile.h"
#include "ui/cimgui.h"
#include "ui/cimgui_ext.h"

static cvar_t cv_basedir = { cvar_text, "basedir", "data", "base directory for game data" };
static cvar_t cv_game = { cvar_text, "game", "id1", "name of the active game" };

static dict_t ms_assets;
static folder_t ms_folder;

void asset_sys_init(void)
{
    cvar_reg(&cv_basedir);
    cvar_reg(&cv_game);

    dict_t assets;
    dict_new(&assets, sizeof(asset_t), EAlloc_Perm);

    char path[PIM_PATH];
    SPrintf(ARGS(path), "%s/%s", cv_basedir.value, cv_game.value);
    folder_t folder = folder_load(path, EAlloc_Perm);

    for (i32 i = 0; i < folder.length; ++i)
    {
        const pack_t* pack = folder.packs + i;
        const u8* packBase = (const u8*)(pack->header);
        for (i32 j = 0; j < pack->filecount; ++j)
        {
            const dpackfile_t* file = pack->files + j;
            const asset_t asset =
            {
                .length = file->length,
                .pData = packBase + file->offset,
            };

            if (!dict_add(&assets, file->name, &asset))
            {
                dict_set(&assets, file->name, &asset);
            }
        }
    }

    ms_assets = assets;
    ms_folder = folder;
}

ProfileMark(pm_update, asset_sys_update)
void asset_sys_update()
{
    ProfileBegin(pm_update);

    ProfileEnd(pm_update);
}

void asset_sys_shutdown(void)
{
    dict_del(&ms_assets);
    folder_free(&ms_folder);
}

ProfileMark(pm_get, asset_get)
bool asset_get(const char* name, asset_t* asset)
{
    ProfileBegin(pm_get);

    ASSERT(name);
    ASSERT(asset);
    bool got = dict_get(&ms_assets, name, asset);

    ProfileEnd(pm_get);
    return got;
}

// ----------------------------------------------------------------------------

typedef enum
{
    FileCmp_Index,
    FileCmp_Name,
    FileCmp_Offset,
    FileCmp_Size,
    FileCmp_UsagePct,

    FileCmp_COUNT
} FileCmpMode;

typedef enum
{
    AssetCmp_Name,
    AssetCmp_Size,

    AssetCmp_COUNT
} AssetCmpMode;

static FileCmpMode gs_fileCmpMode;
static AssetCmpMode gs_assetCmpMode;
static bool gs_revSort;

static i32 CmpFile(const void* lhs, const void* rhs, void* usr)
{
    i32 cmp;
    const dpackfile_t* lFile = lhs;
    const dpackfile_t* rFile = rhs;
    switch (gs_fileCmpMode)
    {
    default:
    case FileCmp_Index:
        cmp = lFile < rFile ? -1 : 1;
        break;
    case FileCmp_Name:
        cmp = StrCmp(ARGS(lFile->name), rFile->name);
        break;
    case FileCmp_Offset:
        cmp = lFile->offset - rFile->offset;
        break;
    case FileCmp_Size:
    case FileCmp_UsagePct:
        cmp = lFile->length - rFile->length;
        break;
    }
    return gs_revSort ? -cmp : cmp;
}

static i32 CmpAsset(
    const char* lKey, const char* rKey,
    const void* lVal, const void* rVal,
    void* usr)
{
    i32 cmp;
    const asset_t* lhs = lVal;
    const asset_t* rhs = rVal;
    switch (gs_assetCmpMode)
    {
    default:
    case AssetCmp_Name:
        cmp = StrCmp(lKey, PIM_PATH, rKey);
        break;
    case AssetCmp_Size:
        cmp = lhs->length - rhs->length;
        break;
    }
    return gs_revSort ? -cmp : cmp;
}

ProfileMark(pm_OnGui, asset_gui)
void asset_gui(void)
{
    ProfileBegin(pm_OnGui);

    igBegin("AssetSystem", NULL, 0);

    if (igCollapsingHeader1("Packs"))
    {
        igIndent(0.0f);
        const i32 numPacks = ms_folder.length;
        const pack_t* packs = ms_folder.packs;
        for (i32 i = 0; i < numPacks; ++i)
        {
            if (!igCollapsingHeader1(packs[i].path))
            {
                continue;
            }

            igPushIDStr(packs[i].path);

            const i32 fileCount = packs[i].filecount;
            const dpackfile_t* files = packs[i].files;
            i32 used = 0;
            for (i32 i = 0; i < fileCount; ++i)
            {
                used += files[i].length;
            }
            const i32 overhead = (sizeof(dpackfile_t) * fileCount) + sizeof(dpackheader_t);
            const i32 empty = (packs[i].bytes - used) - overhead;
            const dpackheader_t* hdr = packs[i].header;

            igValueInt("File Count", fileCount);
            igValueInt("Bytes", packs[i].bytes);
            igValueInt("Used", used);
            igValueInt("Empty", empty);
            igValueInt("Header Offset", hdr->offset);
            igValueInt("Header Length", hdr->length);
            igText("Header ID: %.4s", hdr->id);

            const char* titles[] =
            {
                "Index",
                "Name",
                "Offset",
                "Size",
                "Usage %",
            };
            if (igTableHeader(NELEM(titles), titles, (i32*)&gs_fileCmpMode))
            {
                gs_revSort = !gs_revSort;
            }

            const double rcpUsed = 100.0 / used;
            i32* indices = indsort(files, fileCount, sizeof(files[0]), CmpFile, NULL);
            for (i32 j = 0; j < fileCount; ++j)
            {
                const i32 k = indices[j];
                const dpackfile_t* file = files + k;
                const double len = file->length;
                igText("%d", k); igNextColumn();
                igText("%s", file->name); igNextColumn();
                igText("%d", file->offset); igNextColumn();
                igText("%d", file->length); igNextColumn();
                igText("%2.2f%%", len * rcpUsed); igNextColumn();
            }
            pim_free(indices);
            igTableFooter();

            igPopID();
        }
        igUnindent(0.0f);
    }
    if (igCollapsingHeader1("Assets"))
    {
        const char* titles[] =
        {
            "Name",
            "Size",
        };
        if (igTableHeader(NELEM(titles), titles, (i32*)&gs_assetCmpMode))
        {
            gs_revSort = !gs_revSort;
        }

        const dict_t dict = ms_assets;
        const asset_t* assets = dict.values;
        u32* indices = dict_sort(&dict, CmpAsset, NULL);
        for (u32 i = 0; i < dict.count; ++i)
        {
            u32 j = indices[i];
            const char* name = dict.keys[j];
            ASSERT(name);
            igText(name); igNextColumn();
            igText("%d", assets[j].length); igNextColumn();
        }
        pim_free(indices);

        igTableFooter();
    }

    igEnd();

    ProfileEnd(pm_OnGui);
}
