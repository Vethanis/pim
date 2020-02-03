#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "common/strhash.h"

using HashKey = u32;

namespace HStr
{
    inline constexpr HashKey Hash(cstrc src)
    {
        return StrHash32(src);
    }
};

struct HashString
{
    HashKey Value;

    constexpr HashString()
        : Value(0) {}

    constexpr HashString(HashKey hash)
        : Value(hash) {}

    constexpr HashString(cstrc src)
        : Value(HStr::Hash(src)) {}

    bool IsNull() const { return Value == 0; }
    bool IsNotNull() const { return Value != 0; }
    bool operator==(HashString other) const { return Value == other.Value; }
    bool operator!=(HashString other) const { return Value != other.Value; }
    bool operator<(HashString other) const { return Value < other.Value; }
};
