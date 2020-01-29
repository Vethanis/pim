#pragma once

#include "common/guid.h"
#include "containers/hash_set.h"

static constexpr Comparator<Guid> GuidComparator = { Equals, Compare, Hash };

using GuidSet = HashSet<Guid, GuidComparator>;
