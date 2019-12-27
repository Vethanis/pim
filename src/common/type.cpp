#include "common/type.h"

#include "containers/hash_dict.h"

static constexpr auto GuidComparator = OpComparator<Guid>();
static HashDict<Guid, TypeInfo, GuidComparator> ms_table = { Alloc_Pool };

namespace TypeRegistry
{
    void Register(Slice<const TypeInfo> types)
    {
        for (const TypeInfo& type : types)
        {
            ms_table[type.Id] = type;
        }
    }

    const TypeInfo* Get(Guid guid)
    {
        ASSERT(!IsNull(guid));
        return ms_table.Get(guid);
    }
};
