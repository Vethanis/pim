#include "quake/q_packfile.h"
#include "io/fmap.h"
#include "io/fnd.h"
#include "common/stringutil.h"
#include "allocator/allocator.h"

#include <string.h>

pack_t pack_load(const char* path, EAlloc allocator)
{
    ASSERT(path);

    pack_t pack = { 0 };

    fd_t fd = fd_open(path, 0);
    if (!fd_isopen(fd))
    {
        return pack;
    }

    fmap_t map;
    fmap_create(&map, fd, 0);
    fd_close(&fd);

    if (!map.ptr)
    {
        return pack;
    }

    pack.path = StrDup(path, allocator);
    pack.mapped = map;
    const dpackheader_t* header = map.ptr;

    ASSERT(map.size >= sizeof(*header));
    ASSERT(memcmp(header->id, "PACK", 4) == 0);
    ASSERT(header->length >= 0);

    const u8* buffer = map.ptr;
    pack.files = (const dpackfile_t*)(buffer + header->offset);
    pack.filecount = header->length / sizeof(dpackfile_t);

    return pack;
}

void pack_free(pack_t* pack)
{
    fmap_destroy(&(pack->mapped));
    pack->filecount = 0;
    pack->files = NULL;
    pim_free(pack->path);
    pack->path = NULL;
}

folder_t folder_load(const char* path, EAlloc allocator)
{
    ASSERT(path);

    char packDir[PIM_PATH];
    SPrintf(ARGS(packDir), "%s/*.pak", path);
    StrPath(ARGS(packDir));

    pack_t* packs = NULL;
    i32 length = 0;

    fnd_t fnd = { -1 };
    fnd_data_t fndData;
    while (fnd_iter(&fnd, &fndData, packDir))
    {
        char subdir[PIM_PATH];
        SPrintf(ARGS(subdir), "%s/%s", path, fndData.name);
        StrPath(ARGS(subdir));
        pack_t pack = pack_load(subdir, allocator);
        if (fmap_isopen(pack.mapped))
        {
            ++length;
            packs = pim_realloc(allocator, packs, sizeof(packs[0]) * length);
            packs[length - 1] = pack;
        }
    }

    folder_t folder = { 0 };
    if (length > 0)
    {
        folder.path = StrDup(path, allocator);
        folder.packs = packs;
        folder.length = length;
    }

    return folder;
}

void folder_free(folder_t* folder)
{
    const i32 len = folder->length;
    pack_t* packs = folder->packs;
    for (i32 i = 0; i < len; ++i)
    {
        pack_free(packs + i);
    }
    pim_free(folder->packs);
    folder->packs = NULL;
    folder->length = 0;
    pim_free(folder->path);
    folder->path = NULL;
}
