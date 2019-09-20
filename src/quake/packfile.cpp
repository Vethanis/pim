#include "quake/packfile.h"
#include "common/io.h"
#include <string.h>
#include <stdio.h>

namespace Quake
{
    DeclareNS(SearchPaths);
    DeclareNS(Packs);
    DeclareNS(PackFiles);

    bool LoadPackFile(cstrc dir, Packs& packs, Array<DPackFile>& arena)
    {
        Check0(dir);

        IO::FD fd = IO::Open(dir, IO::OBinSeqRead);
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

        packs.names.grow() = HashString(NS_Packs, dir);
        packs.handles.grow() = fd;
        guard.Cancel();

        PackFiles& files = packs.files.grow();
        files.names.resize(numPackFiles);
        files.chunks.resize(numPackFiles);
        for (i32 i = 0; i < numPackFiles; ++i)
        {
            files.names[i] = HashString(NS_PackFiles, arena[i].name);
            files.chunks[i] = arena[i].chunk;
        }

        return true;
    }

    bool AddGameDirectory(cstr dir, SearchPaths& searchPaths, Array<DPackFile>& arena)
    {
        Check0(dir);

        searchPaths.names.grow() = HashString(NS_SearchPaths, dir);
        Packs& packs = searchPaths.packs.grow();

        IO::FindData fdata = {};
        IO::Finder fder = {};
        char packDir[MaxOsPath];
        sprintf_s(packDir, "%s/*.pak", dir);
        while (IO::Find(fder, fdata, packDir))
        {
            sprintf_s(packDir, "%s/%s", dir, fdata.name);
            LoadPackFile(packDir, packs, arena);
        }

        return true;
    }
};
