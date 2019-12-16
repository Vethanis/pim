#include "components/component.h"
#include "containers/array.h"

static Array<i32> ms_sizes;

namespace ComponentRegistry
{
    void Register(ComponentId id, i32 size)
    {
        while (id.Value >= ms_sizes.Size())
        {
            ms_sizes.Grow() = 0;
        }
        ms_sizes[id.Value] = size;
    }

    i32 Size(ComponentId id)
    {
        return ms_sizes[id.Value];
    }
};
