#include "assets/asset_system.h"

#include "quake/packfile.h"
#include "ui/cimgui.h"
#include "threading/task.h"
#include "common/cvar.h"
#include "common/hashstring.h"
#include "allocator/allocator.h"
#include "common/stringutil.h"
#include <string.h>

static cvar_t cv_imgui;

static void OnGui();

static i32 ms_numAssets;
static u32* ms_hashes;
static asset_t* ms_assets;

static folder_t ms_folder;

void asset_sys_init(void)
{
    cvar_create(&cv_imgui, "asset_imgui", "0.0");

    ms_numAssets = 0;
    ms_hashes = NULL;
    ms_assets = NULL;

    folder_t folder = folder_load("packs/id1", EAlloc_Perm);
    for(i32 i = 0; i < folder.length; ++i)
    {
        const pack_t* pack = folder.packs + i;
        const u8* packBase = (const u8*)(pack->header);
        for (i32 j = 0; j < pack->filecount; ++j)
        {
            const dpackfile_t* file = pack->files + j;
            const asset_t asset =
            {
                file->name,
                pack->path,
                file->offset,
                file->length,
                packBase + file->offset,
            };

            const u32 hash = HashStr(asset.name);
            const i32 k = HashFind(ms_hashes, ms_numAssets, hash);
            const i32 back = ms_numAssets;
            if (k == -1)
            {
                const i32 newLen = back + 1;
                ms_hashes = (u32*)perm_realloc(ms_hashes, sizeof(hash) * newLen);
                ms_assets = (asset_t*)perm_realloc(ms_assets, sizeof(asset) * newLen);
                ms_numAssets = newLen;
            }
            else
            {
                // overwrites asset with modded version when k != -1
                // ensure it isn't actually a hash collision
                ASSERT(StrCmp(asset.name, NELEM(file->name), ms_assets[k].name) == 0);
            }
            ms_hashes[back] = hash;
            ms_assets[back] = asset;
        }
    }
    ms_folder = folder;
}

void asset_sys_update()
{
    if (cv_imgui.asFloat > 0.0f)
    {
        OnGui();
    }
}

void asset_sys_shutdown(void)
{
    ms_numAssets = 0;
    pim_free(ms_assets);
    ms_assets = NULL;
    pim_free(ms_hashes);
    ms_hashes = NULL;
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

i32 asset_sys_get(const char* name, asset_t* asset)
{
    ASSERT(name);
    ASSERT(asset);
    const u32 hash = HashStr(name);
    const i32 i = HashFind(ms_hashes, ms_numAssets, hash);
    if (i != -1)
    {
        memcpy(asset, ms_assets + i, sizeof(asset_t));
        return 1;
    }
    return 0;
}
