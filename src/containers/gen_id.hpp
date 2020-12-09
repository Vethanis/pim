#pragma once

#include "common/macro.h"

struct GenId
{
    u32 version : 8;
    u32 index : 24;

    i32 GetIndex() const { return index; }
    bool operator==(GenId other) const
    {
        return (version == other.version) && (index == other.index);
    }
};
