#pragma once

#include "common/int_types.h"
#include "common/hashstring.h"
#include "common/io.h"
#include "containers/array.h"

namespace Quake
{
    // might want to grow this to MAX_PATH
    static constexpr i32 MaxOsPath = 128;
    static constexpr i32 MaxQPath = 64;

    struct PackFileTag {};
    struct PackTag {};
    struct SearchPathTag {};

    // dpackheader_t
    // common.c 1231
    // must match exactly for binary compatibility
    struct DPackHeader
    {
        char id[4]; // "PACK"
        IO::Chunk chunk;
    };

    // dpackfile_t
    // common.c 1225
    // must match exactly for binary compatibility
    struct DPackFile
    {
        char name[56];
        IO::Chunk chunk;
    };

    // several packfile_t's (chunk + name) inside a pack_t (.pak)
    struct PackFiles
    {
        Array<HashString> names;
        Array<IO::Chunk> chunks;
    };

    // several pack_t's (.pak) inside a searchpath_t (directory)
    struct Packs
    {
        Array<HashString> names;
        Array<IO::FD> handles;
        Array<PackFiles> files;
    };

    // several searchpath_t's (directories) holding Packs
    struct SearchPaths
    {
        Array<HashString> names;
        Array<Packs> packs;
    };

    // COM_LoadPackFile
    // common.c 1619
    // dir: explicit path to .pak
    bool LoadPackFile(cstrc dir, Packs& packs, Array<DPackFile>& arena);

    // COM_AddGameDirectory
    // common.c 1689
    // dir: explicit path to a folder holding .pak files
    bool AddGameDirectory(cstr dir, SearchPaths& searchPaths, Array<DPackFile>& arena);
};
