#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "os/atomics.h"

struct TypeData
{
    i32 sizeOf;
    i32 alignOf;
};

struct TypeId
{
    isize handle;

    bool operator==(TypeId rhs) const { return handle == rhs.handle; }
    bool operator<(TypeId rhs) const { return handle < rhs.handle; }

    bool IsNull() const { return handle == 0; }

    TypeData AsData() const
    {
        ASSERT(!IsNull());
        return *reinterpret_cast<const TypeData*>(handle);
    }
};

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
    return TypeId { TypeMgr::PerType<T>::GetHandle() };
}

template<typename T>
static TypeId TypeOf<const T>()
{
    return TypeOf<T>();
}

template<typename T>
static TypeId TypeOf<const T&>()
{
    return TypeOf<T>();
}

template<typename T>
static TypeId TypeOf<T&>()
{
    return TypeOf<T>();
}

template<typename T>
static TypeId TypeOf<T&&>()
{
    return TypeOf<T>();
}
