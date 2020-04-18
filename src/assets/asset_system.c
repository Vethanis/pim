#include "assets/asset_system.h"

#include "quake/packfile.h"
#include "ui/cimgui.h"
#include "common/cvar.h"
#include "common/hashstring.h"
#include "allocator/allocator.h"
#include "common/stringutil.h"
#include "containers/dict.h"

static cvar_t cv_basedir = { "basedir", "data", "base directory for game data" };
static cvar_t cv_game = { "game", "id1", "name of the active game" };
static cvar_t cv_assetgui = { "assetgui", "1", "show asset system gui" };

static void OnGui();

static dict_t ms_assets;
static folder_t ms_folder;

void asset_sys_init(void)
{
    cvar_reg(&cv_basedir);
    cvar_reg(&cv_game);
    cvar_reg(&cv_assetgui);

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
                .name = file->name,
                .offset = file->offset,
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

void asset_sys_update()
{
    if (cv_assetgui.asFloat != 0.0f)
    {
        OnGui();
    }
}

void asset_sys_shutdown(void)
{
    dict_del(&ms_assets);
    folder_free(&ms_folder);
}

static void OnGui()
{
    igSetNextWindowSize((ImVec2){ 800.0f, 600.0f }, ImGuiCond_Once);
    igBegin("AssetSystem", NULL, 0);
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
        igText("Header ID: %c%c%c%c", hdr->id[0], hdr->id[1], hdr->id[2], hdr->id[3]);
        igSeparator();

        igColumns(5, NULL, true);
        for (i32 j = 0; j < fileCount; ++j)
        {
            const dpackfile_t* file = files + j;
            igText("%d", j); igNextColumn();
            igText("%s", file->name); igNextColumn();
            igText("%d", file->offset); igNextColumn();
            igText("%d", file->length); igNextColumn();
            igText("%2.2f%%", (file->length * 100.0f) / (float)used); igNextColumn();
        }
        igColumns(1, NULL, true);

        igPopID();
    }
    igEnd();
}

bool asset_sys_get(const char* name, asset_t* asset)
{
    ASSERT(name);
    ASSERT(asset);
    return dict_get(&ms_assets, name, asset);
}
