#pragma once

#include "common/macro.h"
#include "common/stringutil.h"
#include "containers/hash.hpp"

template<i32 t_capacity>
class Text
{
private:
    i32 m_length;
    char m_value[t_capacity];
public:
    bool InRange(i32 i) const { return (u32)i < (u32)m_length; }
    i32 Capacity() const { return t_capacity - 1; }
    i32 Size() const { return m_length; }

    char* pim_noalias begin() { return &m_value[0]; }
    char* pim_noalias end() { return &m_value[0] + m_length; }
    char const* pim_noalias begin() const { return &m_value[0]; }
    char const* pim_noalias end() const { return &m_value[0] + m_length; }
    char& pim_noalias operator[](i32 i) { ASSERT(InRange(i)); return m_value[i]; }
    char const& pim_noalias operator[](i32 i) const { ASSERT(InRange(i)); return m_value[i]; }

    Text() : m_length(0), m_value() {}
    explicit Text(char const *const str) : m_length(0), m_value()
    {
        if (str && str[0])
        {
            m_length = StrCpy(ARGS(m_value), str);
        }
    }
};

template<i32 lcap, i32 rcap>
inline bool operator==(const Text<lcap>& lhs, const Text<rcap>& rhs)
{
    return (lhs.Size() == rhs.Size()) &&
        (memcmp(lhs.begin(), rhs.begin(), lhs.Size()) == 0);
}
template<i32 lcap, i32 rcap>
inline bool operator<(const Text<lcap>& lhs, const Text<rcap>& rhs)
{
    return StrCmp(lhs.begin(), lcap, rhs.begin()) < 0;
}

template<i32 cap>
struct Hashable< Text<cap> >
{
    static u32 HashOf(const Text<cap>& item)
    {
        return Hashable<const char*>::HashOf(item.begin());
    }
};
