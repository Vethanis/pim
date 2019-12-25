#include "common/type.h"

#include "containers/dict.h"

static DictTable<256, Guid, TypeInfo> ms_table;

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
