#include "quake/packfile.h"
#include "common/io.h"
#include "common/stringutil.h"

namespace Quake
{
    Pack LoadPack(cstrc dir, EResult& err)
    {
        ASSERT(dir);
        Pack retval = {};

        IO::FDGuard file = IO::Open(dir, IO::OBinSeqRead, err);
        if (!IO::IsOpen(file))
        {
            return retval;
        }

        DPackHeader header;
        i32 nRead = IO::Read(file, &header, sizeof(header), err);
        if (nRead != sizeof(header))
        {
            err = EFail;
            return retval;
        }
        ASSERT(memcmp(header.id, "PACK", 4) == 0);

        const i32 count = header.length / sizeof(DPackFile);
        ASSERT(count > 0);

        IO::Seek(file, header.offset, err);
        ASSERT(err == ESuccess);

        StrCpy(ARGS(retval.path), dir);
        retval.files = { Alloc_Pool };
        retval.files.Resize(count);

        nRead = IO::Read(file, retval.files.begin(), header.length, err);
        ASSERT(err == ESuccess);
        ASSERT(nRead == header.length);

        err = ESuccess;
        return retval;
    }

    void FreePack(Pack& pack)
    {
        pack.files.Reset();
    }

    void LoadFolder(cstrc dir, Array<Pack>& results)
    {
        ASSERT(dir);

        char packDir[PIM_PATH];
        SPrintf(ARGS(packDir), "%s/*.pak", dir);
        FixPath(ARGS(packDir));

        IO::FindData fdata = {};
        IO::Finder fder = {};
        EResult err = EUnknown;
        while (IO::Find(fder, fdata, packDir, err))
        {
            SPrintf(ARGS(packDir), "%s/%s", dir, fdata.name);
            FixPath(ARGS(packDir));
            Pack pack = LoadPack(packDir, err);
            if (err == ESuccess)
            {
                results.Grow() = pack;
            }
        }
    }

    void FreeFolder(Array<Pack>& packs)
    {
        for (Pack& pack : packs)
        {
            FreePack(pack);
        }
        packs.Reset();
    }
};
