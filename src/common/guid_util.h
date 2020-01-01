#pragma once

#include "common/guid.h"
#include "containers/hash_set.h"
#include "containers/lookup_table.h"

struct GuidComparator
{
    static constexpr auto Value = OpComparator<Guid>();
};

using GuidSet = HashSet<Guid, GuidComparator::Value>;
using GuidTable = LookupTable<Guid, GuidComparator::Value>;
