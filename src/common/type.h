#pragma once

#include "common/guid.h"
#include "containers/slice.h"
#include "containers/array.h"

struct FieldInfo
{
    Guid Id;
    cstr Name;
    i32 Offset;
};

struct FuncInfo
{
    Guid Id;
    cstr Name;
    void* Ptr;
};

struct TypeInfo
{
    Guid Id;
    cstr Name;
    i32 Size;
    i32 Align;
    void(*Initializer)(void*);
    void(*Finalizer)(void*);
    Array<FieldInfo> Fields;
    Array<FuncInfo> Funcs;
};

namespace TypeRegistry
{
    void Register(Slice<const TypeInfo> types);
    const TypeInfo* Get(Guid guid);
};
