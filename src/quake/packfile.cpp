#include "quake/packfile.h"
#include "common/io.h"
#include "common/stringutil.h"

namespace Quake
{
    Pack LoadPack(cstrc path, AllocType allocator)
    {
        EResult err = EUnknown;
        ASSERT(path);

        Pack pack = {};
        pack.files.Init(allocator);
        StrCpy(ARGS(pack.path), path);

        IO::StreamGuard file = IO::FOpen(path, "rb", err);
        if (err != ESuccess)
        {
            return pack;
        }

        IO::FRead(file, &pack.header, sizeof(pack.header), err);
        if (err != ESuccess)
        {
            return pack;
        }

        const i64 fileSize = IO::FSize(file, err);
        if (err != ESuccess)
        {
            return pack;
        }
        ASSERT(fileSize < 0x7fffffff);
        pack.size = (i32)fileSize;

        ASSERT(memcmp(pack.header.id, "PACK", 4) == 0);

        IO::FSeek(file, pack.header.offset, err);
        if (err != ESuccess)
        {
            return pack;
        }

        ASSERT(pack.header.length >= 0);
        pack.files.Resize(pack.header.length / sizeof(DPackFile));

        IO::FRead(file, pack.files.begin(), pack.header.length, err);
        ASSERT(err == ESuccess);

        return pack;
    }

    void FreePack(Pack& pack)
    {
        pack.files.Reset();
    }

    Folder LoadFolder(cstrc path, AllocType allocator)
    {
        ASSERT(path);

        Folder folder = {};
        folder.packs.Init(allocator);
        strcpy_s(folder.path, path);

        char packDir[PIM_PATH];
        SPrintf(ARGS(packDir), "%s/*.pak", path);
        FixPath(ARGS(packDir));

        Array<IO::FindData> files = CreateArray<IO::FindData>(Alloc_Stack, 16);
        IO::FindAll(files, packDir);

        for (const IO::FindData& file : files)
        {
            SPrintf(ARGS(packDir), "%s/%s", path, file.name);
            FixPath(ARGS(packDir));
            Pack pack = LoadPack(packDir, allocator);
            if (pack.files.size() > 0)
            {
                folder.packs.Grow() = pack;
            }
        }

        files.Reset();

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
