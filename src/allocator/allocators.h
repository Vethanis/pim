#pragma once

#include "allocator/allocator_vtable.h"
#include "allocator/stdlib_allocator.h"
#include "allocator/linear_allocator.h"
#include "allocator/stack_allocator.h"
#include "allocator/pool_allocator.h"

namespace Allocator
{
    union UAllocator
    {
        Stdlib asStdlib;
        Linear asLinear;
        Stack asStack;
        Pool asPool;
    };

    static constexpr const VTable VTables[] =
    {
        Stdlib::Table,
        Linear::Table,
        Stack::Table,
        Pool::Table,
    };
};
