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

    pack->mapped = FileMap_Open(path, false);
    if (!FileMap_IsOpen(&pack->mapped))
    {
        goto onfail;
    }

    const dpackheader_t* header = pack->mapped.ptr;
    if (pack->mapped.size < sizeof(*header))
    {
        Con_Logf(LogSev_Error, "pak", "Pack is smaller than its header: '%d' in '%s'.", pack->mapped.size, path);
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

    StrCpy(ARGS(pack->path), path);
    pack->files = (const dpackfile_t*)((u8*)(pack->mapped.ptr) + header->offset);
    pack->filecount = filecount;
    return true;

onfail:
    Pack_Free(pack);
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

bool LooseFile_Load(LooseFile* file, const char* dir, const char* name)
{
    memset(file, 0, sizeof(*file));

    file->mapped = FileMap_Open(dir, false);
    if (!FileMap_IsOpen(&file->mapped))
    {
        goto onfail;
    }
    StrCpy(ARGS(file->path), dir);
    StrCpy(ARGS(file->name), name);
    return true;

onfail:
    LooseFile_Free(file);
    return false;
}

void LooseFile_Free(LooseFile* file)
{
    if (file)
    {
        FileMap_Close(&file->mapped);
        memset(file, 0, sizeof(*file));
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
            LooseFile_Free(&sp->files[i]);
        }
        Mem_Free(sp->files);
        memset(sp, 0, sizeof(*sp));
    }
}

i32 SearchPath_FindPack(SearchPath* sp, const char* path)
{
    const i32 ct = sp->packCount;
    const Pack* packs = sp->packs;
    for (i32 i = ct - 1; i >= 0; --i)
    {
        if (StrICmp(ARGS(packs[i].path), path) == 0)
        {
            return i;
        }
    }
    return -1;
}

// not happy with how this looks tbh.
#if PLAT_WIN
#define PACK_GLOB "*.pak"
#else
#define PACK_GLOB "*.[pP][aA][kK]"
#endif

bool SearchPath_AddPack(SearchPath* sp, const char* path)
{
    Pack* packs = sp->packs;
    i32 length = sp->packCount;

    Finder fnd = { { -1 } };
    while (Finder_Iterate(&fnd, path, PACK_GLOB))
    {
        if (SearchPath_FindPack(sp, fnd.relPath) >= 0)
        {
            continue;
        }
        Pack pack;
        if (Pack_Load(&pack, fnd.relPath))
        {
            ++length;
            Perm_Reserve(packs, length);
            packs[length - 1] = pack;
        }
    }

    i32 numLoaded = length - sp->packCount;
    sp->packs = packs;
    sp->packCount = length;

    return numLoaded > 0;
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

i32 SearchPath_FindLooseByPath(SearchPath* sp, const char* path)
{
    ASSERT(path && path[0]);
    const i32 ct = sp->fileCount;
    const LooseFile* files = sp->files;
    for (i32 i = ct - 1; i >= 0; --i)
    {
        if (StrICmp(ARGS(files[i].path), path) == 0)
        {
            return i;
        }
    }
    return -1;
}

i32 SearchPath_FindLooseByName(SearchPath* sp, const char* name)
{
    ASSERT(name && name[0]);
    const i32 ct = sp->fileCount;
    const LooseFile* files = sp->files;
    for (i32 i = ct - 1; i >= 0; --i)
    {
        if (StrICmp(ARGS(files[i].name), name) == 0)
        {
            return i;
        }
    }
    return -1;
}

bool SearchPath_AddLoose(SearchPath* sp, const char* path, const char* name)
{
    // TODO: normalize paths and names, hash them for faster lookup.
    if (SearchPath_FindLooseByPath(sp, path) >= 0)
    {
        return false;
    }
    if (SearchPath_FindLooseByName(sp, path) >= 0)
    {
        return false;
    }
    LooseFile file = { 0 };
    if (LooseFile_Load(&file, path, name))
    {
        i32 b = sp->fileCount++;
        Perm_Reserve(sp->files, b + 1);
        sp->files[b] = file;
        return true;
    }
    return false;
}

bool SearchPath_RmLooseByPath(SearchPath* sp, const char* path)
{
    i32 i = SearchPath_FindLooseByPath(sp, path);
    if (i >= 0)
    {
        LooseFile_Free(&sp->files[i]);
        PopSwap(sp->files, i, sp->fileCount);
        sp->fileCount--;
        return true;
    }
    return false;
}

bool SearchPath_RmLooseByName(SearchPath* sp, const char* name)
{
    i32 i = SearchPath_FindLooseByName(sp, name);
    if (i >= 0)
    {
        LooseFile_Free(&sp->files[i]);
        PopSwap(sp->files, i, sp->fileCount);
        sp->fileCount--;
        return true;
    }
    return false;
}
