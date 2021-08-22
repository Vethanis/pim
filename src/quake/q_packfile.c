#include "quake/q_packfile.h"
#include "io/fmap.h"
#include "io/fnd.h"
#include "common/stringutil.h"
#include "common/console.h"
#include "allocator/allocator.h"

#include <string.h>

bool Pack_Load(Pack* pack, const char* path)
{
    memset(pack, 0, sizeof(*pack));

    FileMap map = FileMap_Open(path, false);
    if (!FileMap_IsOpen(&map))
    {
        goto onfail;
    }

    StrCpy(ARGS(pack->path), path);
    const dpackheader_t* header = map.ptr;

    if (map.size < sizeof(*header))
    {
        Con_Logf(LogSev_Error, "pak", "Pack is smaller than its header: '%d' in '%s'.", map.size, path);
        goto onfail;
    }
    if (memcmp(header->id, "PACK", 4) != 0)
    {
        char id[5] = { 0 };
        memcpy(id, header->id, sizeof(header->id));
        Con_Logf(LogSev_Error, "pak", "Bad pack header id '%s' in '%s'.", id, path);
        goto onfail;
    }
    i32 filecount = header->length / sizeof(dpackfile_t);
    i32 filerem = header->length % sizeof(dpackfile_t);
    if ((header->length < 0) || filerem)
    {
        Con_Logf(LogSev_Error, "pak", "Bad pack header length '%d' in '%s'.", header->length, path);
        goto onfail;
    }

    pack->mapped = map;
    pack->files = (const dpackfile_t*)((u8*)map.ptr + header->offset);
    pack->filecount = filecount;
    return true;

onfail:
    FileMap_Close(&map);
    return false;
}

void Pack_Free(Pack* pack)
{
    if (pack)
    {
        FileMap_Close(&pack->mapped);
        memset(pack, 0, sizeof(*pack));
    }
}

void SearchPath_New(SearchPath* sp)
{
    memset(sp, 0, sizeof(*sp));
}

void SearchPath_Del(SearchPath* sp)
{
    if (sp)
    {
        for (i32 i = 0; i < sp->packCount; ++i)
        {
            Pack_Free(&sp->packs[i]);
        }
        Mem_Free(sp->packs);
        for (i32 i = 0; i < sp->fileCount; ++i)
        {
            Mem_Free(sp->filenames[i]);
        }
        Mem_Free(sp->filenames);
        memset(sp, 0, sizeof(*sp));
    }
}

i32 SearchPath_AddPack(SearchPath* sp, const char* path)
{
    char packDir[PIM_PATH];
    SPrintf(ARGS(packDir), "%s/*.pak", path);
    StrPath(ARGS(packDir));

    Pack* packs = sp->packs;
    i32 length = sp->packCount;

    Finder fnd = { -1 };
    FinderData fndData;
    while (Finder_Iterate(&fnd, &fndData, packDir))
    {
        char subdir[PIM_PATH];
        SPrintf(ARGS(subdir), "%s/%s", path, fndData.name);
        StrPath(ARGS(subdir));
        if (SearchPath_FindPack(sp, subdir) >= 0)
        {
            continue;
        }
        Pack pack;
        if (Pack_Load(&pack, subdir))
        {
            ++length;
            Perm_Reserve(packs, length);
            packs[length - 1] = pack;
        }
    }

    i32 numLoaded = length - sp->packCount;
    sp->packs = packs;
    sp->packCount = length;

    return numLoaded;
}

bool SearchPath_RmPack(SearchPath* sp, const char* path)
{
    i32 i = SearchPath_FindPack(sp, path);
    if (i >= 0)
    {
        Pack_Free(&sp->packs[i]);
        sp->packs[i] = sp->packs[sp->packCount - 1];
        sp->packCount--;
    }
    return i >= 0;
}

i32 SearchPath_FindPack(SearchPath* sp, const char* path)
{
    const i32 len = sp->packCount;
    const Pack* packs = sp->packs;
    for (i32 i = 0; i < len; ++i)
    {
        if (StrICmp(ARGS(packs[i].path), path) == 0)
        {
            return i;
        }
    }
    return -1;
}
