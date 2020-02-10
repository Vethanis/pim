#pragma once

#include "common/guid.h"
#include "common/text.h"
#include "containers/heap.h"
#include "containers/obj_table.h"

struct PackArgs
{
    cstr path;
    i32 fileSize;
};

struct Pack
{
    PathText path;

    Pack(PackArgs args) : path(args.path)
    {

    }
    ~Pack()
    {

    }
};

using PackTable = ObjTable<Guid, Pack, PackArgs>;
