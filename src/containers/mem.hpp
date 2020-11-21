#pragma once

#include "common/macro.h"
#include <string.h>

template<typename T>
pim_inline i32 MemCmp(const T& lhs, const T& rhs)
{
    return memcmp(&lhs, &rhs, sizeof(T));
}

template<typename T>
pim_inline bool operator==(const T& lhs, const T& rhs)
{
    return memcmp(&lhs, &rhs, sizeof(T)) == 0;
}

template<typename T>
pim_inline bool operator!=(const T& lhs, const T& rhs)
{
    return memcmp(&lhs, &rhs, sizeof(T)) != 0;
}

template<typename T>
pim_inline bool operator<(const T& lhs, const T& rhs)
{
    return memcmp(&lhs, &rhs, sizeof(T)) < 0;
}

template<typename T>
pim_inline bool operator<=(const T& lhs, const T& rhs)
{
    return memcmp(&lhs, &rhs, sizeof(T)) <= 0;
}

template<typename T>
pim_inline bool operator>(const T& lhs, const T& rhs)
{
    return memcmp(&lhs, &rhs, sizeof(T)) > 0;
}

template<typename T>
pim_inline bool operator>=(const T& lhs, const T& rhs)
{
    return memcmp(&lhs, &rhs, sizeof(T)) >= 0;
}
