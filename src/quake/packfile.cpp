#include "quake/packfile.h"
#include "common/io.h"
#include <string.h>
#include <stdio.h>

namespace Quake
{
    bool LoadPackFile(cstrc dir, Packs& packs, Array<DPackFile>& arena)
    {
        Check0(dir);

        IO::FD fd = IO::Open(dir, { IO::OBinSeqRead });
        if (!IO::IsOpen(fd))
        {
            return false;
        }
        IO::FDGuard guard(fd);

        DPackHeader header;
        i32 nRead = IO::Read(fd, &header, sizeof(header));
        CheckE0();
        Check0(nRead == sizeof(header));
        Check0(!memcmp(header.id, "PACK", 4));

        const i32 numPackFiles = header.chunk.length / sizeof(DPackFile);
        Check0(numPackFiles > 0);
        arena.resize(numPackFiles);

        IO::Seek(fd, header.chunk.offset);
        CheckE0();

        nRead = IO::Read(fd, arena.begin(), header.chunk.length);
        CheckE0();
        Check0(nRead == header.chunk.length);

        packs.names.grow() = PackName::Add(dir);
        packs.handles.grow() = fd;
        guard.Cancel();

        PackFiles& files = packs.files.grow();
        files.names.resize(numPackFiles);
        files.chunks.resize(numPackFiles);
        for (i32 i = 0; i < numPackFiles; ++i)
        {
            files.names[i] = PackFileName::Add(arena[i].name);
            files.chunks[i] = arena[i].chunk;
        }

        return true;
    }

    bool AddGameDirectory(cstr dir, SearchPaths& searchPaths, Array<DPackFile>& arena)
    {
        Check0(dir);

        searchPaths.names.grow() = SearchPathName::Add(dir);
        Packs& packs = searchPaths.packs.grow();

        char packDir[MaxOsPath];
        for (i32 i = 0; ; ++i)
        {
            sprintf_s(packDir, "%s/pak%i.pak", dir, i);
            CheckE0();
            if (!LoadPackFile(packDir, packs, arena))
            {
                break;
            }
        }

        return true;
    }
};
