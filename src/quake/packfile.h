#pragma once

#include "common/int_types.h"
#include "common/hashstring.h"
#include "common/io.h"
#include "containers/array.h"
#include "math/vec_types.h"

namespace Quake
{
    // dpackheader_t
    // common.c 1231
    // must match exactly for binary compatibility
    struct DPackHeader
    {
        char id[4]; // "PACK"
        i32 offset;
        i32 length;
    };

    // dpackfile_t
    // common.c 1225
    // must match exactly for binary compatibility
    struct DPackFile
    {
        char name[56];
        i32 offset;
        i32 length;
    };

    struct PackFile
    {
        Allocation content;
        i32 refCount;
        i32 offset;
        i32 length;
    };

    struct Pack
    {
        HashString name;
        IO::FD descriptor;
        Array<HashString> filenames;
        Array<PackFile> files;
    };

    struct Folder
    {
        HashString name;
        Array<Pack> packs;
    };

    // COM_LoadPackFile
    // common.c 1619
    // dir: explicit path to .pak
    Pack LoadPack(cstrc dir, Array<DPackFile>& arena, EResult& err);
    void FreePack(Pack& pack);

    // COM_AddGameDirectory
    // common.c 1689
    // dir: explicit path to a folder holding .pak files
    Folder LoadFolder(cstrc dir, Array<DPackFile>& arena, EResult& err);
    void FreeFolder(Folder& folder);
};
