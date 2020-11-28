#pragma once

#include "containers/hash.hpp"

class Hash64 final
{
private:
    u64 m_hash;
public:
    Hash64() : m_hash(0) {}
    explicit Hash64(char const* const str)
    {
        u64 hash = 0;
        if (str && str[0])
        {
            hash = Fnv64String(str, Fnv64Bias);
            hash = hash ? hash : 1;
        }
        m_hash = hash;
    }
    explicit Hash64(void const* const data, i32 bytes)
    {
        u64 hash = 0;
        if (data && bytes > 0)
        {
            hash = Fnv64Bytes(data, bytes, Fnv64Bias);
            hash = hash ? hash : 1;
        }
        m_hash = hash;
    }
    bool operator==(Hash64 other) const { return m_hash == other.m_hash; }
    bool operator!=(Hash64 other) const { return m_hash != other.m_hash; }
    bool operator<(Hash64 other) const { return m_hash < other.m_hash; }
    bool IsNull() const { return m_hash == 0; }
    u64 Value() const { return m_hash; }
};

template<>
struct Hashable<Hash64>
{
    static u32 HashOf(Hash64 const& item)
    {
        return hashutil_create_hash(Fnv32Qword(item.Value(), Fnv32Bias));
    }
};
