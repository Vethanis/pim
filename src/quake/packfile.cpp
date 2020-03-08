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
        StrCpy(ARGS(pack.path), path);

        IO::FD fd = IO::Open(path, IO::OBinary, err);
        if (!IO::IsOpen(fd))
        {
            return pack;
        }

        IO::FileMap map = IO::Map(fd, 0, 0, IO::OBinary);
        IO::Close(fd, err);

        if (!IO::IsOpen(map))
        {
            return pack;
        }

        pack.ptr = map.memory.begin();
        pack.bytes = map.memory.size();
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
            IO::FileMap map = { (u8*)pack.ptr, pack.bytes };
            IO::Unmap(map);
            pack.ptr = 0;
            pack.header = 0;
            pack.files = 0;
        }
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

        Array<IO::FindData> files = CreateArray<IO::FindData>(Alloc_Temp, 16);
        IO::FindAll(files, packDir);

        for (const IO::FindData& file : files)
        {
            SPrintf(ARGS(packDir), "%s/%s", path, file.name);
            FixPath(ARGS(packDir));
            Pack pack = LoadPack(packDir, allocator);
            if (pack.ptr != NULL)
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
