#include "quake/packfile.h"
#include "common/io.h"
#include "common/stringutil.h"

namespace Quake
{
    Pack LoadPack(cstrc dir, AllocType allocator)
    {
        ASSERT(dir);

        Pack pack = {};
        IO::FileMap file = {};

        StrCpy(ARGS(pack.path), dir);
        pack.files.Init(allocator);

        file = IO::MapFile(dir, false);
        if (!file.address)
        {
            return pack;
        }

        Slice<u8> memory = file.AsSlice();
        Slice<DPackHeader> headers = memory.Cast<DPackHeader>();
        if (headers.Size() < 1)
        {
            return pack;
        }

        DPackHeader header = headers[0];

        ASSERT(memcmp(header.id, "PACK", 4) == 0);
        ASSERT((header.length % sizeof(DPackFile)) == 0);

        Slice<DPackFile> metadata = memory
            .Subslice(header.offset, header.length)
            .Cast<DPackFile>();

        pack.files.Copy(metadata);

        IO::Close(file);

        return pack;
    }

    void FreePack(Pack& pack)
    {
        pack.files.Reset();
    }

    Folder LoadFolder(cstrc dir, AllocType allocator)
    {
        ASSERT(dir);

        Folder folder = {};
        folder.packs.Init(allocator);
        StrCpy(ARGS(folder.path), dir);

        char packDir[PIM_PATH];
        SPrintf(ARGS(packDir), "%s/*.pak", dir);
        FixPath(ARGS(packDir));

        Array<IO::FindData> files = { Alloc_Linear };
        IO::FindAll(files, packDir);

        for (const IO::FindData& file : files)
        {
            SPrintf(ARGS(packDir), "%s/%s", dir, file.name);
            FixPath(ARGS(packDir));
            Pack pack = LoadPack(packDir, allocator);
            if (pack.files.Size() > 0)
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
