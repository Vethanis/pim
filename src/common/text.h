#pragma once

#include "common/int_types.h"

template<i32 t_capacity>
struct Text
{
    static constexpr i32 Capacity = t_capacity;
    char value[Capacity];
};
