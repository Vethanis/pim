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

        EResult err = EUnknown;
        IO::FD fd = IO::Open(dir, IO::OBinary, err);
        if (err == EFail)
        {
            return pack;
        }

        file = IO::MapFile(fd, false, err);
        if (err == EFail)
        {
            IO::Close(fd, err);
            return pack;
        }

        pack.size = file.memory.size();
        Slice<DPackHeader> headers = file.memory.Cast<DPackHeader>();
        if (headers.size() < 1)
        {
            IO::Close(file);
            return pack;
        }

        pack.header = headers[0];

        ASSERT(memcmp(pack.header.id, "PACK", 4) == 0);
        ASSERT((pack.header.length % sizeof(DPackFile)) == 0);

        Slice<DPackFile> metadata = file.memory
            .Subslice(pack.header.offset, pack.header.length)
            .Cast<DPackFile>();

        Copy(pack.files, metadata);

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
