#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "common/memory.h"

template<typename T>
inline bool CmpEq(const T& lhs, const T& rhs)
{
    return MemEq(&lhs, &rhs, sizeof(T));
}

template<typename T>
inline bool CmpLt(const T& lhs, const T& rhs)
{
    return MemCmp(&lhs, &rhs, sizeof(T)) < 0;
}

template<typename T>
inline bool CmpGt(const T& lhs, const T& rhs)
{
    return MemCmp(&lhs, &rhs, sizeof(T)) > 0;
}

template<typename T>
inline bool CmpLtEq(const T& lhs, const T& rhs)
{
    return MemCmp(&lhs, &rhs, sizeof(T)) <= 0;
}

template<typename T>
inline bool CmpGtEq(const T& lhs, const T& rhs)
{
    return MemCmp(&lhs, &rhs, sizeof(T)) >= 0;
}
