#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "containers/array.h"

template<typename T > struct rm_const          { typedef T type; };
template<typename T > struct rm_const<const T> { typedef T type; };

struct TypeIndex
{
    u32 Value;

    inline operator u32 () const
    {
        return Value;
    }
};

struct TypeInfo
{
    TypeIndex index;
    void(*DropFn)(void*);
    cstr name;
    u32 sizeOf;
    u32 alignOf;
    bool isPOD;

    static Array<TypeInfo> ms_lookup;

    inline static bool IsValid(TypeIndex index)
    {
        return ms_lookup.in_range(index - 1);
    }
    inline static TypeInfo Lookup(TypeIndex index)
    {
        DebugAssert(IsValid(index));
        return ms_lookup[index - 1];
    }
    inline static TypeIndex Create()
    {
        return { ms_lookup.size() + 1u };
    }
    inline static void Register(TypeInfo info)
    {
        DebugAssert(info.index == (ms_lookup.size() + 1));
        DebugAssert(info.DropFn != nullptr);
        DebugAssert(info.name != nullptr);
        DebugAssert(info.sizeOf != 0);
        DebugAssert(info.alignOf != 0);
        ms_lookup.grow() = info;
    }
};

template<typename T>
struct TypeOfHelper
{
    static TypeInfo ms_info;

    inline TypeOfHelper(cstrc name, bool isPOD, void (*DropFn)(void* pVoid))
    {
        if (ms_info.index == 0)
        {
            ms_info.index = TypeInfo::Create();
            ms_info.DropFn = DropFn;
            ms_info.name = name;
            ms_info.sizeOf = sizeof(T);
            ms_info.alignOf = alignof(T);
            ms_info.isPOD = isPOD;
            TypeInfo::Register(ms_info);
        }
    }

    inline static TypeIndex Index()
    {
        DebugAssert(ms_info.index != 0);
        return ms_info.index;
    }

    inline static TypeInfo Info()
    {
        DebugAssert(ms_info.index != 0);
        return ms_info;
    }
};

template<typename T>
TypeInfo TypeOfHelper<T>::ms_info;

#define Reflect(T, isPOD, DropExpr) static TypeOfHelper<T> _TOH_##T = TypeOfHelper<T>(#T, isPOD, [](void* pVoid){ T* ptr = (T*)pVoid; DropExpr; });

template<typename T>
inline TypeInfo TypeOf() { return TypeOfHelper< rm_const<T>::type >::Info(); }

inline TypeInfo TypeOf(TypeIndex index) { return TypeInfo::Lookup(index); }

template<typename T>
inline TypeIndex TypeId() { return TypeOfHelper<T>::Index(); }