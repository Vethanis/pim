#pragma once

#include "common/int_types.h"
#include "containers/array.h"

struct RfType;
struct RfMember;
struct RfItems;

struct RfType
{
    usize id;
    cstr name;
    u32 size;
    u32 align;
    const RfType* deref;      // for pointers
    Array<RfMember> members;  // for structs
};

struct RfMember
{
    cstr name;
    u32 offset;
    const RfType* value;
};

struct RfItems
{
    RfType type;
    void* compArrays;
};

template<typename T>
static constexpr usize RfId();

template<typename T>
static constexpr const RfType& RfGet();

static RfItems& RfGetItems(usize id);

static const RfType& RfGet(usize id);

#define Reflect(T) static const ValueReflector<T> r_##T(#T, "const " #T, "" #T "*", "const " #T "*");
#define ReflectMember(T, name) static const MemberReflector<T, decltype(T##::##name)> rm_##T##_##name(#name, OffsetOf(T, name));

// ----------------------------------------------------------------------------

template<typename T>
struct RfStore
{
    static void Create(cstr name, const RfType* deref)
    {
        ms_items.type = { RfId<T>(), name, sizeof(T), alignof(T), deref, { 0, 0 } };
    }
    static void AddMember(cstr name, u32 offset, const RfType* desc)
    {
        ms_items.type.members.grow() = { name, offset, desc };
    }

    static RfItems ms_items;
};

template<typename T>
static constexpr usize RfId() { return (usize)(&RfStore<T>::ms_items); }

template<typename T>
static constexpr const RfType& RfGet() { return RfStore<T>::ms_items.type; }

static RfItems& RfGetItems(usize id)
{
    return *(RfItems*)id;
}

static const RfType& RfGet(usize id)
{
    return ((const RfItems*)id)->type;
}

template<typename T>
struct ValueReflector
{
    ValueReflector(cstr name, cstr cname, cstr ptrName, cstr cptrName)
    {
        RfStore<T>::Create(name, 0);
        RfStore<const T>::Create(cname, 0);
        RfStore<T*>::Create(ptrName, &RfGet<T>());
        RfStore<const T*>::Create(cptrName, &RfGet<const T>());
    }
};

template<typename T, typename U>
struct MemberReflector
{
    MemberReflector(cstr name, u32 offset)
    {
        RfStore<T>::AddMember(name, offset, &RfGet<U>());
        RfStore<const T>::AddMember(name, offset, &RfGet<U>());
    }
};

struct RfTest
{
    u32 a;
    const u32 b;
    u32* c;
    const u32* d;
};
