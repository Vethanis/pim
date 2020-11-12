#pragma once

#include "common/macro.h"
#include "common/fnv1a.h"

class Hash32 final
{
private:
    u32 m_hash;
public:
    Hash32() : m_hash(0) {}
    Hash32(const Hash32& other) : m_hash(other.m_hash) {}
    explicit Hash32(const char* str)
    {
        u32 hash = 0;
        if (str && str[0])
        {
            hash = Fnv32String(str, Fnv32Bias);
            hash = hash ? hash : 1;
        }
        m_hash = hash;
    }
    explicit Hash32(const void* data, i32 bytes)
    {
        u32 hash = 0;
        if (data && bytes > 0)
        {
            hash = Fnv32Bytes(data, bytes, Fnv32Bias);
            hash = hash ? hash : 1;
        }
        m_hash = hash;
    }
    Hash32& operator=(const Hash32& other)
    {
        m_hash = other.m_hash;
        return *this;
    }
    bool operator==(Hash32 other) const
    {
        return m_hash == other.m_hash;
    }
    bool operator!=(Hash32 other) const
    {
        return m_hash != other.m_hash;
    }
    bool operator<(Hash32 other) const
    {
        return m_hash < other.m_hash;
    }
    bool IsNull() const
    {
        return m_hash == 0;
    }
};
