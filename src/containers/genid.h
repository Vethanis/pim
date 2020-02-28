#pragma once

#include "common/macro.h"
#include "common/int_types.h"

struct GenId
{
public:
    GenId() : Value(0) {}
    GenId(i32 index, u8 version)
    {
        u32 i = index;
        u32 v = version;
        ASSERT(i <= kIndexMask);
        Value = (v << kVersionShift) | (i & kIndexMask);
    }

    i32 GetIndex() const { return (i32)(Value & kIndexMask); }
    u8 GetVersion() const { return (u8)((Value >> kVersionShift) & 0xffu); }
    void SetIndex(i32 index) { *this = GenId(index, GetVersion()); }
    void SetVersion(u8 version) { *this = GenId(GetIndex(), version); }

    bool IsNull() const { return Value == 0u; }
    bool IsNotNull() const { return Value != 0u; }
    bool operator==(GenId rhs) const { return Value == rhs.Value; }
    bool operator!=(GenId rhs) const { return Value != rhs.Value; }
    bool operator<(GenId rhs) const { return Value < rhs.Value; }

private:
    static constexpr u32 kVersionShift = 24u;
    static constexpr u32 kVersionMask  = 0xff000000u;
    static constexpr u32 kIndexMask    = 0x00ffffffu;

    u32 Value;
};
SASSERT(sizeof(GenId) == sizeof(u32));
