#include "quake/packfile.h"
#include "io/fmap.h"
#include "io/fnd.h"
#include "common/stringutil.h"
#include "allocator/allocator.h"
#include <string.h>

pack_t pack_load(const char* path, EAlloc allocator)
{
    ASSERT(path);

    pack_t pack;
    memset(&pack, 0, sizeof(pack));
    StrCpy(ARGS(pack.path), path);

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

    pack.header = map.ptr;
    pack.bytes = map.size;

    ASSERT(pack.bytes >= sizeof(pack.header));
    ASSERT(memcmp(pack.header->id, "PACK", 4) == 0);
    ASSERT(pack.header->length >= 0);

    const u8* buffer = map.ptr;
    pack.files = (const dpackfile_t*)(buffer + pack.header->offset);
    pack.filecount = pack.header->length / sizeof(dpackfile_t);

    return pack;
}

void pack_free(pack_t* pack)
{
    if (pack->header)
    {
        void* ptr = (void*)(pack->header);
        fmap_t map = { ptr, pack->bytes };
        fmap_destroy(&map);
        pack->header = NULL;
        pack->files = NULL;
        pack->filecount = 0;
        pack->bytes = 0;
    }
}

folder_t folder_load(const char* path, EAlloc allocator)
{
    ASSERT(path);

    folder_t folder;
    memset(&folder, 0, sizeof(folder));

    StrCpy(ARGS(folder.path), path);

    char packDir[PIM_PATH];
    SPrintf(ARGS(packDir), "%s/*.pak", path);
    StrPath(ARGS(packDir));

    fnd_t fnd = { -1 };
    fnd_data_t fndData;
    while (fnd_iter(&fnd, &fndData, packDir))
    {
        char subdir[PIM_PATH];
        SPrintf(ARGS(subdir), "%s/%s", path, fndData.name);
        StrPath(ARGS(subdir));
        pack_t pack = pack_load(subdir, allocator);
        if (pack.header != NULL)
        {
            i32 back = folder.length++;
            folder.packs = pim_realloc(allocator, folder.packs, sizeof(pack_t) * (back + 1));
            folder.packs[back] = pack;
        }
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
}
