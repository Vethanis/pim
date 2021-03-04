#pragma once

#include "containers/hash.hpp"
#include "common/compiletimehash.hpp"

class Hash32 final
{
private:
    u32 m_hash;
public:
    Hash32() : m_hash(0) {}
    explicit constexpr Hash32(char const* const str) : m_hash(CompileTimeHash32(str)) {}
    explicit Hash32(void const* const data, i32 bytes) : m_hash(HashBytes(data, bytes)) {}
    bool operator==(Hash32 other) const { return m_hash == other.m_hash; }
    bool operator!=(Hash32 other) const { return m_hash != other.m_hash; }
    bool operator<(Hash32 other) const { return m_hash < other.m_hash; }
    bool IsNull() const { return m_hash == 0; }
    bool IsValid() const { return m_hash != 0; }
    u32 Value() const { return m_hash; }
};

template<>
struct Hashable<Hash32>
{
    static u32 HashOf(Hash32 const& item)
    {
        return hashutil_create_hash(item.Value());
    }
};
