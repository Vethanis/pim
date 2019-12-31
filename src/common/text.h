#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "common/hash.h"
#include "common/stringutil.h"

template<i32 t_capacity>
struct Text
{
    static constexpr i32 Capacity = t_capacity;
    SASSERT(Capacity > 0);

    char value[Capacity];

    // ------------------------------------------------------------------------

    Text() : value() { value[0] = '\0'; }
    Text(cstrc src) : value()
    {
        value[0] = '\0';
        if (src)
        {
            StrCpy(value, Capacity, src);
        }
    }

    // ------------------------------------------------------------------------

    inline i32 capacity() const { return Capacity; }
    inline i32 size() const { return StrLen(value, Capacity); }

    inline cstr begin() const { return value; }
    inline cstr end() const { return value + size(); }

    inline char* begin() { return value; }
    inline char* end() { return value + size(); }

    inline operator cstr () const { return value; }

    // ------------------------------------------------------------------------

    inline static i32 Compare(const Text& lhs, const Text& rhs)
    {
        return StrCmp(lhs.value, Capacity, rhs.value);
    }

    inline static i32 ICompare(const Text& lhs, const Text& rhs)
    {
        return StrICmp(lhs.value, Capacity, rhs.value);
    }

    inline static bool Equals(const Text& lhs, const Text& rhs)
    {
        return Compare(lhs, rhs) == 0;
    }

    inline static bool IEquals(const Text& lhs, const Text& rhs)
    {
        return ICompare(lhs, rhs) == 0;
    }

    inline static u32 Hash(const Text& text)
    {
        return Fnv32String(text);
    }
};
