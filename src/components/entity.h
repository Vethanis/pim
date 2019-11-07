#pragma once

#include "common/macro.h"
#include "common/int_types.h"

// ----------------------------------------------------------------------------

enum TableId : u16
{
    TableId_Default = 0,
    TableId_Map,
    TableId_Props,
    TableId_Characters,
    TableId_Weapons,
    TableId_Projectiles,
    TableId_UI,

    TableId_Count
};

// ----------------------------------------------------------------------------

struct Entity
{
    TableId table;
    u16 version;
    u16 index;

    inline bool IsNull() const { return version == 0; }
    inline bool IsNotNull() const { return version != 0; }
};
