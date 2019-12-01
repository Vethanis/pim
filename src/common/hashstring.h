#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "common/strhash.h"

using HashKey = u64;

namespace HStr
{
    inline constexpr HashKey Hash(cstrc src)
    {
        return StrHash64(src);
    }

#if _DEBUG
    cstr Lookup(HashKey key);
    void Insert(HashKey key, cstrc value);
#endif // _DEBUG

}; // HStr

struct HashString
{
    HashKey Value;

    inline constexpr HashString()
        : Value(0) {}

    inline constexpr HashString(HashKey hash)
        : Value(hash) {}

#if _DEBUG
    inline HashString(cstrc src)
        : Value(HStr::Hash(src))
    {
        HStr::Insert(Value, src);
    }
#else
    inline constexpr HashString(cstrc src)
        : Value(HStr::Hash(src)) {}
#endif // _DEBUG

    inline bool IsNull() const { return Value == 0; }
    inline bool IsNotNull() const { return Value != 0; }
    inline bool operator==(HashString other) const { return Value == other.Value; }
    inline bool operator!=(HashString other) const { return Value != other.Value; }

    inline cstr DebugGet() const
    {
        IfDebug(return HStr::Lookup(Value));
        IfNotDebug(return nullptr);
    }

}; // HashString

