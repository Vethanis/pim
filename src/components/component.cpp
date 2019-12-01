#include "components/component.h"

namespace Component
{
    static i32 ms_sizes[ComponentType_Count];
    static DropFn ms_dropFns[ComponentType_Count];

    void SetSize(ComponentType type, i32 size)
    {
        Assert(ValidType(type));
        Assert(size > 0);
        Assert(ms_sizes[type] == 0);
        ms_sizes[type] = size;
    }

    i32 SizeOf(ComponentType type)
    {
        Assert(ValidType(type));
        Assert(ms_sizes[type] > 0);
        return ms_sizes[type];
    }

    void SetDropFn(ComponentType type, DropFn fn)
    {
        Assert(ValidType(type));
        Assert(fn != nullptr);
        Assert(ms_dropFns[type] == nullptr);
        ms_dropFns[type] = fn;
    }

    void Drop(ComponentType type, void* pVoid)
    {
        Assert(ValidType(type));
        Assert(pVoid != nullptr);
        Assert(ms_dropFns[type] != nullptr);
        ms_dropFns[type](pVoid);
    }

};
