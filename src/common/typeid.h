#pragma once

#include "common/int_types.h"

template<typename Family>
struct TypeIds
{
    static i32 ms_counter;

    template<typename Instance>
    static i32 Get()
    {
        static const i32 s_id = ms_counter++;
        return s_id;
    }

    static i32 Count()
    {
        return ms_counter;
    }
};

template<typename Family>
i32 TypeIds<Family>::ms_counter;

