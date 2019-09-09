#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "common/hash.h"
#include "containers/dict.h"
#include "common/tolower.h"

struct Text
{
    static constexpr i32 Capacity = 128;
    char value[Capacity];
};
using TextDict = Dict<u32, Text>;

static constexpr u32 StrHash(cstrc src)
{
    if (!src) { return 0; }

    u32 hash = Fnv32_Bias;
    i32 i = 0;
    constexpr i32 len = Text::Capacity - 1;
    for (; (i < len) && (src[i]); ++i)
    {
        hash = Fnv32Byte(ToLower(src[i]), hash);
    }
    hash = hash ? hash : 1u;

    return hash;
}

// TODO: NatVis
template<typename T>
struct HashString
{
    u32 m_hash;

    inline HashString() : m_hash(0) {}
    inline HashString(u32 x) : m_hash(x) {}
    inline bool IsNull() const { return !m_hash; }
    inline cstr Get() const
    {
        const i32 i = ms_dict.m_keys.rfind(m_hash);
        return (i == -1) ? 0 : ms_dict.m_values[i].value;
    }
    HashString(cstrc src)
    {
        m_hash = 0;
        if (!src)
        {
            return;
        }

        Text txt;
        u32 hash = Fnv32_Bias;
        {
            i32 i = 0;
            constexpr i32 len = Text::Capacity - 1;
            char* dst = txt.value;
            for (; (i < len) && (src[i]); ++i)
            {
                char c = ToLower(src[i]);
                hash ^= c;
                hash *= Fnv32_Prime;
                dst[i] = c;
            }
            dst[i] = 0;
            hash = hash ? hash : 1u;
        }

        const i32 idx = ms_dict.m_keys.rfind(hash);
        if (idx != -1)
        {
            cstrc A = ms_dict.m_values[idx].value;
            cstrc B = txt.value;
            for (i32 i = 0; A[i]; ++i)
            {
                if (A[i] & B[i])
                {
                    DebugInterrupt();
                    return;
                }
            }
            m_hash = hash;
            return;
        }

        m_hash = hash;
        ms_dict.m_keys.grow() = hash;
        ms_dict.m_values.grow() = txt;
    }

    // unique id / string registration
    static HashString Add(cstr src)
    {
        Check(src, return { 0u });
        u32 hash = Fnv32_Bias;
        char* dst = ms_dict.m_values.grow().value;
        for (i32 i = 0; (i < Text::Capacity) && src[i]; ++i)
        {
            char c = ToLower(src[i]);
            hash = Fnv32Byte(c, hash);
            dst[i] = c;
        }
        DebugAssert(ms_dict.m_keys.rfind(hash) == -1);
        ms_dict.m_keys.grow() = hash;
        return { hash };
    }

    static void Reset() { ms_dict.reset(); }
    static TextDict ms_dict;
};

template<typename T>
TextDict HashString<T>::ms_dict;

