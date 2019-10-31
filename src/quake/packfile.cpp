#include "quake/packfile.h"
#include "common/io.h"
#include "common/stringutil.h"

namespace Quake
{
    DeclareHashNS(SearchPaths)
    DeclareHashNS(Packs)
    DeclareHashNS(PackFiles)

    bool LoadPackFile(cstrc dir, PackAssets& assets, Array<DPackFile>& arena)
    {
        Check0(dir);

        IO::FD file = IO::Open(dir, IO::OBinSeqRead);
        if (!IO::IsOpen(file))
        {
            return false;
        }
        IO::FDGuard guard(file);

        DPackHeader header;
        i32 nRead = IO::Read(file, &header, sizeof(header));
        Check0(!IO::GetErrNo());
        Check0(nRead == sizeof(header));
        Check0(!StrCmp(argof(header.id), "PACK"));

        const i32 numPackFiles = header.chunk.length / sizeof(DPackFile);
        Check0(numPackFiles > 0);
        arena.resize(numPackFiles);

        IO::Seek(file, header.chunk.offset);
        Check0(!IO::GetErrNo());

        nRead = IO::Read(file, arena.begin(), header.chunk.length);
        Check0(!IO::GetErrNo());
        Check0(nRead == header.chunk.length);

        // add a pack
        {
            const i32 pathId = assets.paths.names.size() - 1;

            assets.packs.names.grow() = HashString(NS_Packs, dir);
            assets.packs.pathIds.grow() = pathId;
            assets.packs.files.grow() = file;
            guard.Cancel();
        }

        // add packfiles
        {
            const i32 packId = assets.packs.names.size() - 1;

            const i32 prevLen = assets.packfiles.names.resizeRel(numPackFiles);
            assets.packfiles.chunks.resizeRel(numPackFiles);
            assets.packfiles.packIds.resizeRel(numPackFiles);

            for (i32 i = 0; i < numPackFiles; ++i)
            {
                const i32 j = prevLen + i;
                assets.packfiles.names[j] = HashString(NS_PackFiles, arena[i].name);
                assets.packfiles.chunks[j] = arena[i].chunk;
                assets.packfiles.packIds[j] = packId;
            }
        }

        return true;
    }

    bool AddGameDirectory(cstrc dir, PackAssets& assets, Array<DPackFile>& arena)
    {
        Check0(dir);

        // add a path
        assets.paths.names.grow() = HashString(NS_SearchPaths, dir);

        IO::FindData fdata = {};
        IO::Finder fder = {};
        char packDir[PIM_MAX_PATH];
        SPrintf(argof(packDir), "%s/*.pak", dir);
        FixPath(argof(packDir));
        while (IO::Find(fder, fdata, packDir))
        {
            SPrintf(argof(packDir), "%s/%s", dir, fdata.name);
            FixPath(argof(packDir));
            LoadPackFile(packDir, assets, arena);
        }

        return true;
    }
};
