#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "containers/slice.h"

struct FieldInfo;
struct TypeInfo;

struct TypeInfo
{
    cstrc Name;
    const usize Size;
    const usize Align;
    const Slice<const FieldInfo> Fields;
    const bool IsPOD;
};

template<typename T>
inline constexpr TypeInfo TypeOf();

template<typename T>
inline constexpr TypeInfo TypeOf(const T& t) { return TypeOf<T>(); }

struct FieldInfo
{
    const TypeInfo Type;
    cstrc Name;
    const usize Offset;
};

template<typename T>
inline constexpr TypeInfo BasicType(const char* const name)
{
    return TypeInfo
    {
        name,
        sizeof(T),
        alignof(T),
        { 0, 0 },
        true,
    };
}

template<typename T>
inline constexpr TypeInfo StructType(
    const char* const name,
    const FieldInfo* const fields, size_t numFields)
{
    bool isPOD = true;
    for (usize i = 0; i < numFields; ++i)
    {
        if (!fields[i].Type.IsPOD)
        {
            isPOD = false;
        }
    }
    return TypeInfo
    {
        name,
        sizeof(T),
        alignof(T),
        { fields, numFields },
        isPOD,
    };
}

template<>
inline constexpr TypeInfo TypeOf<i32>() { return BasicType<i32>("i32"); }

struct Foo final
{
    i32 bar;
    i32 blorp;
};

static constexpr FieldInfo FooFields[] =
{
    { TypeOf<i32>(), "bar", 0 },
    { TypeOf<i32>(), "blorp", 4 },
};

template<>
inline constexpr TypeInfo TypeOf<Foo>()
{
    return StructType<Foo>("Foo", FooFields, 2);
}

static void TypeExample()
{
    i32 x = 5;
    TypeInfo type = TypeOf(x);
    // puts(type.Name);
    TypeInfo fooType = TypeOf<Foo>();
    // puts(fooType.Name);
    for (FieldInfo field : fooType.Fields)
    {
        // puts(field.Name);
    }
}
