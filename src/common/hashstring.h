#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "common/hash.h"
#include "common/stringutil.h"

using HashNS = u8;
using HashKey = u32;

#define DeclareHashNS(T)           static constexpr HashNS NS_##T   = HStr::CreateNs(#T);
#define DeclareHash(name, ns, txt) static constexpr HashKey name    = HStr::Hash(ns, txt);

namespace HStr
{
    constexpr HashKey NsValueBits = 5;
    constexpr HashKey NsTypeBits = sizeof(HashNS) * 8;
    constexpr HashKey NsCount = 1 << NsValueBits;
    constexpr HashKey NsMask = NsCount - 1;

    constexpr HashKey HashTypeBits = sizeof(HashKey) * 8;
    constexpr HashKey HashValueBits = HashTypeBits - NsValueBits;

    StaticAssert(NsTypeBits >= NsValueBits);

    inline constexpr HashNS CreateNs(cstrc name)
    {
        return (HashNS)(Fnv32String(name) & NsMask);
    }

    inline constexpr HashKey EmbedNs(HashNS ns, HashKey hash)
    {
        DebugAssert(NsTypeBits >= NsValueBits);
        return (hash << NsValueBits) | (ns & NsMask);
    }

    inline constexpr HashNS GetNs(HashKey key)
    {
        return key & NsMask;
    }

    inline constexpr HashKey GetHash(HashKey key)
    {
        return key >> NsValueBits;
    }

    inline constexpr HashKey Hash(HashNS ns, cstrc src)
    {
        if (!src || !src[0]) { return 0; }

        HashKey hash = Fnv32Bias;
        for (i32 i = 0; src[i]; ++i)
        {
            hash = Fnv32Byte(ToLower(src[i]), hash);
        }

        return EmbedNs(ns, Max(hash, 1));
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


    inline constexpr HashString(HashNS ns, HashKey hash)
        : Value(HStr::EmbedNs(ns, hash)) {}

#if _DEBUG
    inline HashString(HashNS ns, cstrc src)
        : Value(HStr::Hash(ns, src))
    {
        HStr::Insert(Value, src);
    }
    inline HashString(cstrc src)
        : Value(HStr::Hash(0, src))
    {
        HStr::Insert(Value, src);
    }
#else
    inline constexpr HashString(HashNS ns, cstrc src)
        : Value(HStr::Hash(ns, src)) {}

    inline constexpr HashString(cstrc src)
        : Value(HStr::Hash(0, src)) {}
#endif // _DEBUG

    inline bool IsNull() const { return HStr::GetHash(Value) == 0; }
    inline bool IsNotNull() const { return HStr::GetHash(Value) != 0; }
    inline bool operator==(HashString other) const { return Value == other.Value; }
    inline bool operator!=(HashString other) const { return Value != other.Value; }

    inline cstr DebugGet() const
    {
        IfDebug(return HStr::Lookup(Value));
        IfNotDebug(return nullptr);
    }

}; // HashString

