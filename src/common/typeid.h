#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "common/comparator.h"
#include "os/atomics.h"

struct ComponentRow;

struct TypeData
{
    i32 sizeOf;
    i32 alignOf;
    ComponentRow* pRow;
};

struct TypeId
{
    isize handle;

    bool operator==(TypeId rhs) const { return handle == rhs.handle; }
    bool operator!=(TypeId rhs) const { return handle != rhs.handle; }
    bool operator<(TypeId rhs) const { return handle < rhs.handle; }

    bool IsNull() const { return handle == 0; }
    bool IsNotNull() const { return handle != 0; }

    TypeData& AsData() const
    {
        ASSERT(!IsNull());
        return *reinterpret_cast<TypeData*>(handle);
    }
    i32 SizeOf() const { return AsData().sizeOf; }
    i32 AlignOf() const { return AsData().alignOf; }
};

static bool Equals(const TypeId& lhs, const TypeId& rhs)
{
    return lhs.handle == rhs.handle;
}
static i32 Compare(const TypeId& lhs, const TypeId& rhs)
{
    if (lhs.handle != rhs.handle)
    {
        return lhs.handle < rhs.handle ? -1 : 1;
    }
    return 0;
}
static u32 Hash(const TypeId& id)
{
    return Fnv32Qword(id.handle);
}
static constexpr Comparator<TypeId> TypeIdComparator = { Equals, Compare, Hash };

struct TypeMgr
{
    template<typename T>
    struct PerType
    {
        static TypeData ms_data;

        static isize GetHandle()
        {
            Store(ms_data.sizeOf, (i32)sizeof(T));
            Store(ms_data.alignOf, (i32)alignof(T));
            return reinterpret_cast<isize>(&ms_data);
        }
    };
};

template<typename T>
TypeData TypeMgr::PerType<T>::ms_data;

template<typename T>
static TypeId TypeOf()
{
    return TypeId{ TypeMgr::PerType<T>::GetHandle() };
}
