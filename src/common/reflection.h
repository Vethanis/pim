#pragma once

#include "common/int_types.h"
#include "common/array.h"

template<typename T>
void _TypeFn() {}

template<typename T>
static constexpr usize RfId()
{
    void(*fnPtr)() = _TypeFn<T>;
    return (usize)fnPtr;
}

struct RfType;
struct RfMember;

struct RfType
{
    cstr name;
    u32 size;
    u32 align;
    const RfType* value;      // for pointers
    Array<RfMember> members;  // for structs
};

struct RfMember
{
    cstr name;
    u32 offset;
    const RfType* value;
};

template<typename T>
struct RfStorage
{
    static void Add(const RfMember& x) { Value.members.grow() = x; }

    static RfType Value;
};

template<typename T>
static const RfType& RfGet() { return RfStorage<T>::Value; }

template<typename T>
struct ValueReflector
{
    ValueReflector(cstr name, cstr ptrName)
    {
        const RfType x = { name, sizeof(T), alignof(T), 0, { 0, 0 } };
        RfStorage<T>::Value = x;
        const RfType pX = { ptrName, sizeof(T*), alignof(T*), &RfStorage<T>::Value, { 0, 0 } };
        RfStorage<const T>::Value = x;
        RfStorage<T*>::Value = pX;
        RfStorage<const T*>::Value = pX;
    }
};

template<typename T, typename U>
struct MemberReflector
{
    MemberReflector(cstr name, u32 offset)
    {
        const RfMember x = { name, offset, &RfStorage<U>::Value };
        RfStorage<T>::Add(x);
        RfStorage<const T>::Add(x);
    }
};

#define Reflect(T) static const ValueReflector<T> r_##T(#T, "" #T "*");
#define AddMember(T, name) static const MemberReflector<T, decltype(T##::##name)> rm_##T##_##name(#name, OffsetOf(T, name));

// could be useful if the above hits any snags
//template<typename T>
//struct rem_ptr { using type = T; };
//
//template<typename T>
//struct rem_ptr<const T*> { using type = T; };
//
//template<typename T>
//struct rem_ptr<T*> { using type = T; };
//
//template<typename T>
//struct rem_ptr<const T> { using type = T; };

struct Test
{
    u32 a;
    const u32 b;
    u32* c;
    const u32* d;
};
