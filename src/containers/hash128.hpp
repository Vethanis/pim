#pragma once

#include "common/macro.h"
#include "common/fnv1a.h"

class Hash128 final
{
private:
    u64 m_a;
    u64 m_b;
public:
    Hash128() : m_a(0), m_b(0) {}
    Hash128(const Hash128& other) : m_a(other.m_a), m_b(other.m_b) {}
    explicit Hash128(const char* str)
    {
        u64 a = 0;
        u64 b = 0;
        if (str && str[0])
        {
            a = Fnv64String(str, Fnv64Bias);
            b = Fnv64String(str, a);
            b = b ? b : 1;
        }
        m_a = a;
        m_b = b;
    }
    explicit Hash128(const void* data, i32 bytes)
    {
        u64 a = 0;
        u64 b = 0;
        if (data && bytes > 0)
        {
            a = Fnv64Bytes(data, bytes, Fnv64Bias);
            b = Fnv64Bytes(data, bytes, a);
            b = b ? b : 1;
        }
        m_a = a;
        m_b = b;
    }
    Hash128& operator=(const Hash128& other)
    {
        m_a = other.m_a;
        m_b = other.m_b;
        return *this;
    }
    bool operator==(Hash128 other) const
    {
        return ((m_a - other.m_a) | (m_b - other.m_b)) == 0;
    }
    bool operator!=(Hash128 other) const
    {
        return ((m_a - other.m_a) | (m_b - other.m_b)) != 0;
    }
    bool operator<(Hash128 other) const
    {
        return (m_a < other.m_a) && (m_b < other.m_b);
    }
    bool IsNull() const
    {
        return (m_a | m_b) == 0;
    }
};
