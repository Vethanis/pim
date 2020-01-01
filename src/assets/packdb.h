#pragma once

#include "common/guid.h"
#include "common/guid_util.h"
#include "common/text.h"
#include "containers/array.h"

struct PackDb
{
    struct FilePos
    {
        i32 offset;
        i32 size;
    };

    struct PackAssets
    {
        GuidTable table;            // guid table of asset name
        Array<FilePos> positions;   // position of asset in file
    };

    GuidTable table;            // guid table of the pack path
    Array<PathText> paths;      // path to the pack file
    Array<PackAssets> assets;   // assets within the pack

    // ------------------------------------------------------------------------

    void Init(AllocType allocator);
    void Reset();

    // ------------------------------------------------------------------------

    i32 size() const { return table.size(); }
    const Guid* begin() const { return table.begin(); }
    const Guid* end() const { return table.end(); }

    // ------------------------------------------------------------------------

    Guid& GetName(i32 i) { return table[i]; }
    PathText& GetPath(i32 i) { return paths[i]; }
    PackAssets& GetAssets(i32 i) { return assets[i]; }
};
