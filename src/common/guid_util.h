#pragma once

#include "common/guid.h"
#include "containers/hash_set.h"
#include "containers/lookup_table.h"

static constexpr Comparator<Guid> GuidComparator = { Equals, Compare, Hash };

using GuidSet = HashSet<Guid, GuidComparator>;
using GuidTable = LookupTable<Guid, GuidComparator>;
