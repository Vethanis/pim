#include "quake/packfile.h"
#include "common/io.h"
#include "common/stringutil.h"

namespace Quake
{
    Pack LoadPack(cstrc dir, Array<DPackFile>& arena, EResult& err)
    {
        ASSERT(dir);
        Pack retval = {};
        retval.name = HashString(dir);

        IO::FDGuard file = IO::Open(dir, IO::OBinSeqRead, err);
        CHECKERR();

        DPackHeader header;
        i32 nRead = IO::Read(file, &header, sizeof(header), err);
        CHECKERR();
        CHECK(nRead == sizeof(header));
        CHECK(!memcmp(header.id, "PACK", 4));

        const i32 numPackFiles = header.length / sizeof(DPackFile);
        CHECK(numPackFiles > 0);

        IO::Seek(file, header.offset, err);
        CHECKERR();

        arena.Resize(numPackFiles);
        nRead = IO::Read(file, arena.begin(), header.length, err);
        CHECKERR();
        CHECK(nRead == header.length);

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
            Allocator::Free(file.content.begin());
            file.content = { 0, 0 };
        }
        pack.files.Reset();
    }

    Folder LoadFolder(cstrc dir, Array<DPackFile>& arena, EResult& err)
    {
        ASSERT(dir);
        Folder retval = {};
        retval.name = HashString(dir);

        char packDir[PIM_PATH];
        SPrintf(ARGS(packDir), "%s/*.pak", dir);
        FixPath(ARGS(packDir));

        IO::FindData fdata = {};
        IO::Finder fder = {};
        while (IO::Find(fder, fdata, packDir, err))
        {
            SPrintf(ARGS(packDir), "%s/%s", dir, fdata.name);
            FixPath(ARGS(packDir));
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
