#include "quake/packfile.h"
#include "common/io.h"
#include "common/stringutil.h"

namespace Quake
{
    Pack LoadPack(cstrc path, EAlloc allocator)
    {
        ASSERT(path);

        Pack pack = {};
        StrCpy(ARGS(pack.path), path);

        fd_t fd = pim_open(path, 0);
        if (!fd_isopen(fd))
        {
            return pack;
        }

        fmap_t map = pim_mmap(fd, 0);
        pim_close(&fd);

        if (!map.ptr)
        {
            return pack;
        }

        pack.ptr = (const u8*)map.ptr;
        pack.bytes = map.size;
        pack.header = (const DPackHeader*)pack.ptr;
        ASSERT(pack.bytes >= sizeof(pack.header));

        ASSERT(memcmp(pack.header->id, "PACK", 4) == 0);
        ASSERT(pack.header->length >= 0);
        pack.count = pack.header->length / sizeof(DPackFile);
        const u8* pFileHeaders = pack.ptr + pack.header->offset;
        pack.files = (const DPackFile*)pFileHeaders;

        return pack;
    }

    void FreePack(Pack& pack)
    {
        if (pack.ptr)
        {
            fmap_t map = { (void*)pack.ptr, pack.bytes };
            pim_munmap(&map);
            pack.ptr = 0;
            pack.header = 0;
            pack.files = 0;
        }
    }

    Folder LoadFolder(cstrc path, EAlloc allocator)
    {
        ASSERT(path);

        Folder folder = {};
        folder.packs.Init(allocator);
        StrCpy(ARGS(folder.path), path);

        char packDir[PIM_PATH];
        SPrintf(ARGS(packDir), "%s/*.pak", path);
        StrPath(ARGS(packDir));

        fnd_t fnd = -1;
        fnd_data_t fndData;
        while (pim_fnditer(&fnd, &fndData, packDir))
        {
            char subdir[PIM_PATH];
            SPrintf(ARGS(subdir), "%s/%s", path, fndData.name);
            StrPath(ARGS(subdir));
            Pack pack = LoadPack(subdir, allocator);
            if (pack.ptr != NULL)
            {
                folder.packs.Grow() = pack;
            }
        }

        return folder;
    }

    void FreeFolder(Folder& folder)
    {
        for (Pack& pack : folder.packs)
        {
            FreePack(pack);
        }
        folder.packs.Reset();
    }
};
