#include "components/component.h"

namespace Component
{
    static i32 ms_sizes[ComponentType_Count];
    static DropFn ms_dropFns[ComponentType_Count];

    void SetSize(ComponentType type, i32 size)
    {
        DebugAssert(ValidType(type));
        DebugAssert(size > 0);
        DebugAssert(ms_sizes[type] == 0);
        ms_sizes[type] = size;
    }

    i32 SizeOf(ComponentType type)
    {
        DebugAssert(ValidType(type));
        DebugAssert(ms_sizes[type] > 0);
        return ms_sizes[type];
    }

    void SetDropFn(ComponentType type, DropFn fn)
    {
        DebugAssert(ValidType(type));
        DebugAssert(fn != nullptr);
        DebugAssert(ms_dropFns[type] == nullptr);
        ms_dropFns[type] = fn;
    }

    void Drop(ComponentType type, void* pVoid)
    {
        DebugAssert(ValidType(type));
        DebugAssert(pVoid != nullptr);
        DebugAssert(ms_dropFns[type] != nullptr);
        ms_dropFns[type](pVoid);
    }

};
