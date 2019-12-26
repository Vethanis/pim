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

        DPackFile* pHeaders = Allocator::AllocT<DPackFile>(Alloc_Linear, count);
        nRead = IO::Read(file, pHeaders, header.length, err);
        ASSERT(err == ESuccess);
        ASSERT(nRead == header.length);

        retval.name = HashString(dir);
        retval.descriptor = file.Take();
        retval.count = count;
        retval.filenames = Allocator::AllocT<HashString>(Alloc_Pool, count);
        retval.files = Allocator::AllocT<PackFile>(Alloc_Pool, count);

        for (int i = 0; i < count; ++i)
        {
            retval.filenames[i] = HashString(pHeaders[i].name);
            retval.files[i] = {};
            retval.files[i].offset = pHeaders[i].offset;
            retval.files[i].length = pHeaders[i].length;
        }

        Allocator::Free(pHeaders);

        err = ESuccess;
        return retval;
    }

    void FreePack(Pack& pack)
    {
        EResult err = EUnknown;
        IO::Close(pack.descriptor, err);

        Allocator::Free(pack.filenames);
        pack.filenames = nullptr;

        const i32 count = pack.count;
        PackFile* files = pack.files;
        for(i32 i = 0; i < count; ++i)
        {
            Allocator::Free(files[i].content);
            files[i].content = nullptr;
        }
        Allocator::Free(files);
        pack.files = nullptr;
    }

    Folder LoadFolder(cstrc dir, EResult& err)
    {
        ASSERT(dir);
        Folder retval = {};
        retval.name = HashString(dir);

        char packDir[PIM_PATH];
        SPrintf(ARGS(packDir), "%s/*.pak", dir);
        FixPath(ARGS(packDir));

        IO::FindData fdata = {};
        IO::Finder fder = {};
        Array<Pack> packs = { Alloc_Pool };
        while (IO::Find(fder, fdata, packDir, err))
        {
            SPrintf(ARGS(packDir), "%s/%s", dir, fdata.name);
            FixPath(ARGS(packDir));
            Pack pack = LoadPack(packDir, err);
            if (err == ESuccess)
            {
                packs.Grow() = pack;
            }
        }

        retval.packs = packs.begin();
        retval.count = packs.Size();

        err = retval.count > 0 ? ESuccess : EFail;

        return retval;
    }

    void FreeFolder(Folder& folder)
    {
        const i32 count = folder.count;
        Pack* packs = folder.packs;
        for (i32 i = 0; i < count; ++i)
        {
            FreePack(packs[i]);
        }
        Allocator::Free(packs);
        folder.packs = nullptr;
        folder.count = 0;
    }
};
