#pragma once

#include "containers/hash.hpp"

class Hash128 final
{
private:
    u64 m_a;
    u64 m_b;
public:
    Hash128() : m_a(0), m_b(0) {}
    explicit Hash128(char const* const str)
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
    explicit Hash128(void const* const data, i32 bytes)
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
    bool IsNull() const { return (m_a | m_b) == 0; }
    u64 Alpha() const { return m_a; }
    u64 Beta() const { return m_b; }
};

template<>
struct Hashable<Hash128>
{
    static u32 HashOf(Hash128 const& item)
    {
        u32 hash = Fnv32Bias;
        hash = Fnv32Qword(item.Alpha(), hash);
        hash = Fnv32Qword(item.Beta(), hash);
        return hashutil_create_hash(hash);
    }
};
