#include "assets/asset_system.h"

#include "allocator/allocator.h"
#include "common/cvar.h"
#include "common/fnv1a.h"
#include "common/profiler.h"
#include "common/sort.h"
#include "common/stringutil.h"
#include "common/console.h"
#include "io/fnd.h"
#include "containers/sdict.h"
#include "quake/q_packfile.h"
#include "quake/q_bspfile.h"
#include "ui/cimgui_ext.h"
#include "stb/stb_image.h"

static ConVar_t cv_basedir = { .type = cvart_text,.name = "basedir",.value = "data",.desc = "base directory for game data" };
static ConVar_t cv_gamedir = { .type = cvart_text,.name = "gamedir",.value = "id1",.desc = "name of the active game" };

static sdict_t ms_assets;
static searchpath_t ms_search;

void AssetSys_Init(void)
{
    cvar_reg(&cv_basedir);
    cvar_reg(&cv_gamedir);

    sdict_new(&ms_assets, sizeof(asset_t), EAlloc_Perm);
    searchpath_new(&ms_search);

    char path[PIM_PATH] = { 0 };
    SPrintf(ARGS(path), "%s/%s", cvar_get_str(&cv_basedir), cvar_get_str(&cv_gamedir));
    searchpath_addpack(&ms_search, path);

    for (i32 i = 0; i < ms_search.packCount; ++i)
    {
        const pack_t* pack = &ms_search.packs[i];
        const u8* packBase = pack->mapped.ptr;
        for (i32 j = 0; j < pack->filecount; ++j)
        {
            const dpackfile_t* file = pack->files + j;
            const asset_t asset =
            {
                .length = file->length,
                .pData = packBase + file->offset,
            };

            if (!sdict_add(&ms_assets, file->name, &asset))
            {
                sdict_set(&ms_assets, file->name, &asset);
            }
        }
    }
}

ProfileMark(pm_update, AssetSys_Update)
void AssetSys_Update()
{
    ProfileBegin(pm_update);

    ProfileEnd(pm_update);
}

void AssetSys_Shutdown(void)
{
    sdict_del(&ms_assets);
    searchpath_del(&ms_search);
}

bool Asset_Get(const char* name, asset_t* asset)
{
    ASSERT(name);
    ASSERT(asset);
    return sdict_get(&ms_assets, name, asset);
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
        cmp = StrCmp(lKey, 64, rKey);
        break;
    case AssetCmp_Size:
        cmp = lhs->length - rhs->length;
        break;
    }
    return gs_revSort ? -cmp : cmp;
}

ProfileMark(pm_OnGui, AssetSys_Gui)
void AssetSys_Gui(bool* pEnabled)
{
    ProfileBegin(pm_OnGui);

    if (igBegin("AssetSystem", pEnabled, 0))
    {
        if (igExCollapsingHeader1("Packs"))
        {
            igIndent(0.0f);
            const i32 numPacks = ms_search.packCount;
            const pack_t* packs = ms_search.packs;
            for (i32 i = 0; i < numPacks; ++i)
            {
                const pack_t pack = packs[i];
                if (!igExCollapsingHeader1(pack.path))
                {
                    continue;
                }

                igPushIDStr(pack.path);

                const i32 fileCount = pack.filecount;
                const dpackfile_t* files = pack.files;
                i32 used = 0;
                for (i32 i = 0; i < fileCount; ++i)
                {
                    used += files[i].length;
                }
                const i32 overhead = (sizeof(dpackfile_t) * fileCount) + sizeof(dpackheader_t);
                const i32 empty = (pack.mapped.size - used) - overhead;
                const dpackheader_t* hdr = pack.mapped.ptr;

                igValueInt("File Count", fileCount);
                igValueInt("Bytes", pack.mapped.size);
                igValueInt("Used", used);
                igValueInt("Empty", empty);
                igValueInt("Header Offset", hdr->offset);
                igValueInt("Header Length", hdr->length);
                igText("Header ID: %.4s", hdr->id);

                static const char* const titles[] =
                {
                    "Index",
                    "Name",
                    "Offset",
                    "Size",
                    "Usage %",
                };
                if (igExTableHeader(NELEM(titles), titles, (i32*)&gs_fileCmpMode))
                {
                    gs_revSort = !gs_revSort;
                }

                const double rcpUsed = 100.0 / used;
                i32* indices = indsort(files, fileCount, sizeof(files[0]), CmpFile, NULL);
                for (i32 j = 0; j < fileCount; ++j)
                {
                    const i32 k = indices[j];
                    const dpackfile_t* file = files + k;
                    igText("%d", k); igNextColumn();
                    igText("%s", file->name); igNextColumn();
                    igText("%d", file->offset); igNextColumn();
                    igText("%d", file->length); igNextColumn();
                    igText("%2.2f%%", file->length * rcpUsed); igNextColumn();
                }
                Mem_Free(indices);
                igExTableFooter();

                igPopID();
            }
            igUnindent(0.0f);
        }
        if (igExCollapsingHeader1("Assets"))
        {
            const char* titles[] =
            {
                "Name",
                "Size",
            };
            if (igExTableHeader(NELEM(titles), titles, (i32*)&gs_assetCmpMode))
            {
                gs_revSort = !gs_revSort;
            }

            const sdict_t dict = ms_assets;
            char const *const *const names = dict.keys;
            const asset_t* assets = dict.values;
            u32* indices = sdict_sort(&dict, CmpAsset, NULL);
            for (u32 i = 0; i < dict.count; ++i)
            {
                u32 j = indices[i];
                igText(names[j]); igNextColumn();
                igText("%d", assets[j].length); igNextColumn();
            }
            Mem_Free(indices);

            igExTableFooter();
        }
    }
    igEnd();

    ProfileEnd(pm_OnGui);
}
