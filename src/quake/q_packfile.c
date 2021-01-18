#include "quake/q_packfile.h"
#include "io/fmap.h"
#include "io/fnd.h"
#include "common/stringutil.h"
#include "common/console.h"
#include "allocator/allocator.h"

#include <string.h>

bool pack_load(pack_t* pack, const char* path)
{
    memset(pack, 0, sizeof(*pack));

    fmap_t map = fmap_open(path, false);
    if (!fmap_isopen(map))
    {
        goto onfail;
    }

    StrCpy(ARGS(pack->path), path);
    const dpackheader_t* header = map.ptr;

    if (map.size < sizeof(*header))
    {
        con_logf(LogSev_Error, "pak", "Pack is smaller than its header: '%d' in '%s'.", map.size, path);
        goto onfail;
    }
    if (memcmp(header->id, "PACK", 4) != 0)
    {
        char id[5] = { 0 };
        memcpy(id, header->id, sizeof(header->id));
        con_logf(LogSev_Error, "pak", "Bad pack header id '%s' in '%s'.", id, path);
        goto onfail;
    }
    i32 filecount = header->length / sizeof(dpackfile_t);
    i32 filerem = header->length % sizeof(dpackfile_t);
    if ((header->length < 0) || filerem)
    {
        con_logf(LogSev_Error, "pak", "Bad pack header length '%d' in '%s'.", header->length, path);
        goto onfail;
    }

    pack->mapped = map;
    pack->files = (const dpackfile_t*)((u8*)map.ptr + header->offset);
    pack->filecount = filecount;
    return true;

onfail:
    fmap_close(&map);
    return false;
}

void pack_free(pack_t* pack)
{
    if (pack)
    {
        fmap_close(&pack->mapped);
        memset(pack, 0, sizeof(*pack));
    }
}

void searchpath_new(searchpath_t* sp)
{
    memset(sp, 0, sizeof(*sp));
}

void searchpath_del(searchpath_t* sp)
{
    if (sp)
    {
        for (i32 i = 0; i < sp->packCount; ++i)
        {
            pack_free(&sp->packs[i]);
        }
        pim_free(sp->packs);
        for (i32 i = 0; i < sp->fileCount; ++i)
        {
            pim_free(sp->filenames[i]);
        }
        pim_free(sp->filenames);
        memset(sp, 0, sizeof(*sp));
    }
}

i32 searchpath_addpack(searchpath_t* sp, const char* path)
{
    char packDir[PIM_PATH];
    SPrintf(ARGS(packDir), "%s/*.pak", path);
    StrPath(ARGS(packDir));

    pack_t* packs = sp->packs;
    i32 length = sp->packCount;

    fnd_t fnd = { -1 };
    fnd_data_t fndData;
    while (fnd_iter(&fnd, &fndData, packDir))
    {
        char subdir[PIM_PATH];
        SPrintf(ARGS(subdir), "%s/%s", path, fndData.name);
        StrPath(ARGS(subdir));
        pack_t pack;
        if (pack_load(&pack, subdir))
        {
            ++length;
            PermReserve(packs, length);
            packs[length - 1] = pack;
        }
    }

    i32 numLoaded = length - sp->packCount;
    sp->packs = packs;
    sp->packCount = length;

    return numLoaded;
}

void searchpath_rmpack(searchpath_t* sp, const char* path)
{
    i32 len = sp->packCount;
    pack_t* packs = sp->packs;
    for (i32 i = 0; i < len; ++i)
    {
        if (StrIStr(packs[i].path, PIM_PATH, path))
        {
            pack_free(&packs[i]);
            --len;
            packs[i] = packs[len];
            memset(&packs[len], 0, sizeof(packs[0]));
            --i;
        }
    }
    sp->packCount = len;
    if (!len)
    {
        pim_free(sp->packs);
        sp->packs = NULL;
    }
}
