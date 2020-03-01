#pragma once

#include "common/int_types.h"
#include "containers/slice.h"
#include "threading/task.h"

namespace AssetSystem
{
    bool Exists(cstr name);
    bool IsLoaded(cstr name);
    ITask* CreateLoad(cstr name);
    void FreeLoad(ITask* pLoadTask);
    Slice<u8> Acquire(cstr name);
    void Release(cstr name);
};
