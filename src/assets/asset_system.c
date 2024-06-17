#include "assets/asset_system.h"

#include "allocator/allocator.h"
#include "common/cvars.h"
#include "common/fnv1a.h"
#include "common/profiler.h"
#include "common/sort.h"
#include "common/stringutil.h"
#include "common/console.h"
#include "common/time.h"
#include "io/fnd.h"
#include "containers/sdict.h"
#include "ui/cimgui_ext.h"
#include "stb/stb_image.h"

static StrDict ms_assets;
static char ms_dir[PIM_PATH];

static void GetGameDir(char* dst, i32 size)
{
    SPrintf(dst, size, "%s/%s", ConVar_GetStr(&cv_basedir), ConVar_GetStr(&cv_game));
}

static void RefreshTable(void)
{
    StrDict_Clear(&ms_assets);
}

void AssetSys_Init(void)
{
    StrDict_New(&ms_assets, sizeof(asset_t), EAlloc_Perm);

    GetGameDir(ARGS(ms_dir));
    RefreshTable();
}

ProfileMark(pm_update, AssetSys_Update)
void AssetSys_Update()
{
    ProfileBegin(pm_update);

    char dir[PIM_PATH];
    GetGameDir(ARGS(dir));
    if (StrCmp(ARGS(ms_dir), dir) != 0)
    {
        StrCpy(ARGS(ms_dir), dir);
        RefreshTable();
    }

    ProfileEnd(pm_update);
}

void AssetSys_Shutdown(void)
{
    StrDict_Del(&ms_assets);
}

bool Asset_Get(const char* name, asset_t* asset)
{
    ASSERT(name);
    ASSERT(asset);
    return StrDict_Get(&ms_assets, name, asset);
}

// ----------------------------------------------------------------------------

typedef enum
{
    AssetCmp_Name,
    AssetCmp_Size,

    AssetCmp_COUNT
} AssetCmpMode;

static AssetCmpMode gs_assetCmpMode;
static bool gs_revSort;

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

            const StrDict dict = ms_assets;
            char const *const *const names = dict.keys;
            const asset_t* assets = dict.values;
            u32* indices = StrDict_Sort(&dict, CmpAsset, NULL);
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
