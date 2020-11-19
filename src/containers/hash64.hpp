#pragma once

#include "common/macro.h"
#include "common/fnv1a.h"

class Hash64 final
{
private:
    u64 m_hash;
public:
    Hash64() : m_hash(0) {}
    Hash64(const Hash64& other) : m_hash(other.m_hash) {}
    explicit Hash64(const char* str)
    {
        u64 hash = 0;
        if (str && str[0])
        {
            hash = Fnv64String(str, Fnv64Bias);
            hash = hash ? hash : 1;
        }
        m_hash = hash;
    }
    explicit Hash64(const void* data, i32 bytes)
    {
        u64 hash = 0;
        if (data && bytes > 0)
        {
            hash = Fnv64Bytes(data, bytes, Fnv64Bias);
            hash = hash ? hash : 1;
        }
        m_hash = hash;
    }
    Hash64& operator=(const Hash64& other)
    {
        m_hash = other.m_hash;
        return *this;
    }
    bool operator==(Hash64 other) const
    {
        return m_hash == other.m_hash;
    }
    bool operator!=(Hash64 other) const
    {
        return m_hash != other.m_hash;
    }
    bool operator<(Hash64 other) const
    {
        return m_hash < other.m_hash;
    }
    bool IsNull() const
    {
        return m_hash == 0;
    }
};
