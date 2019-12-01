#include "quake/packfile.h"
#include "common/io.h"
#include "common/stringutil.h"

namespace Quake
{
    Pack LoadPack(cstrc dir, Array<DPackFile>& arena, EResult& err)
    {
        Assert(dir);
        Pack retval = {};
        retval.name = HashString(dir);

        IO::FDGuard file = IO::Open(dir, IO::OBinSeqRead, err);
        CheckErr();

        DPackHeader header;
        i32 nRead = IO::Read(file, &header, sizeof(header), err);
        CheckErr();
        Check(nRead == sizeof(header));
        Check(!memcmp(header.id, "PACK", 4));

        const i32 numPackFiles = header.length / sizeof(DPackFile);
        Check(numPackFiles > 0);

        IO::Seek(file, header.offset, err);
        CheckErr();

        arena.Resize(numPackFiles);
        nRead = IO::Read(file, arena.begin(), header.length, err);
        CheckErr();
        Check(nRead == header.length);

        retval.descriptor = file.Take();
        retval.filenames.Resize(numPackFiles);
        retval.files.Resize(numPackFiles);
        for (int i = 0; i < numPackFiles; ++i)
        {
            retval.filenames[i] = HashString(arena[i].name);
            retval.files[i] = {};
            retval.files[i].offset = arena[i].offset;
            retval.files[i].length = arena[i].length;
        }

        return retval;
    }
    void FreePack(Pack& pack)
    {
        EResult err = EUnknown;
        IO::Close(pack.descriptor, err);
        pack.filenames.Reset();
        for (PackFile& file : pack.files)
        {
            Allocator::Free(file.content);
        }
        pack.files.Reset();
    }

    Folder LoadFolder(cstrc dir, Array<DPackFile>& arena, EResult& err)
    {
        Assert(dir);
        Folder retval = {};
        retval.name = HashString(dir);

        char packDir[PIM_PATH];
        SPrintf(argof(packDir), "%s/*.pak", dir);
        FixPath(argof(packDir));

        IO::FindData fdata = {};
        IO::Finder fder = {};
        while (IO::Find(fder, fdata, packDir, err))
        {
            SPrintf(argof(packDir), "%s/%s", dir, fdata.name);
            FixPath(argof(packDir));
            Pack pack = LoadPack(packDir, arena, err);
            if (err == ESuccess)
            {
                retval.packs.Grow() = pack;
            }
        }

        return retval;
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
