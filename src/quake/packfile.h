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

    struct SearchPaths
    {
        Array<HashString> names;

        inline void Reset()
        {
            names.reset();
        }
    };

    struct Packs
    {
        Array<i32> pathIds;
        Array<HashString> names;
        Array<IO::FD> files;

        inline void Reset()
        {
            pathIds.reset();
            names.reset();
            for (IO::FD& file : files)
            {
                IO::Close(file);
            }
            files.reset();
        }
    };

    struct PackFiles
    {
        Array<i32> packIds;
        Array<HashString> names;
        Array<IO::Chunk> chunks;

        inline void Reset()
        {
            packIds.reset();
            names.reset();
            chunks.reset();
        }
    };

    struct PackAssets
    {
        SearchPaths paths;
        Packs packs;
        PackFiles packfiles;

        inline void Reset()
        {
            paths.Reset();
            packs.Reset();
            packfiles.Reset();
        }
    };

    // COM_LoadPackFile
    // common.c 1619
    // dir: explicit path to .pak
    bool LoadPackFile(cstrc dir, PackAssets& assets, Array<DPackFile>& arena);

    // COM_AddGameDirectory
    // common.c 1689
    // dir: explicit path to a folder holding .pak files
    bool AddGameDirectory(cstrc dir, PackAssets& assets, Array<DPackFile>& arena);
};
