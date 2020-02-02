#pragma once

#include "common/guid.h"
#include "containers/hash_set.h"

using GuidSet = HashSet<Guid, GuidComparator>;

template<typename T>
static bool GuidBasedEqualsFn(const T& lhs, const T& rhs)
{
    return GuidEqualsFn(lhs.guid, rhs.guid);
}

template<typename T>
static i32 GuidBasedCompareFn(const T& lhs, const T& rhs)
{
    return GuidCompareFn(lhs.guid, rhs.guid);
}

template<typename T>
static u32 GuidBasedHashFn(const T& item)
{
    return GuidHashFn(item.guid);
}

template<typename T>
static constexpr Comparator<T> GuidBasedComparator()
{
    return
    {
        GuidBasedEqualsFn,
        GuidBasedCompareFn,
        GuidBasedHashFn,
    };
}
