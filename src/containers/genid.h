#pragma once

#include "common/int_types.h"
#include "common/comparator.h"
#include "common/hash.h"

struct GenId
{
    u32 index;
    u32 version;
};

static bool Equals(const GenId& lhs, const GenId& rhs)
{
    return ((lhs.index - rhs.index) | (lhs.version - rhs.version)) == 0u;
}

static i32 Compare(const GenId& lhs, const GenId& rhs)
{
    if (lhs.index != rhs.index)
    {
        return lhs.index < rhs.index ? -1 : 1;
    }
    if (lhs.version != rhs.version)
    {
        return lhs.version < rhs.version ? -1 : 1;
    }
    return 0;
}

static u32 Hash(const GenId& id)
{
    return Fnv32Dword(id.version, Fnv32Dword(id.index));
}

static constexpr Comparator<GenId> GenIdComparator = { Equals, Compare, Hash };
