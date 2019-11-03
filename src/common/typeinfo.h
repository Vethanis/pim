#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "common/hash.h"
#include <stdio.h>

template<typename T>
struct TypeOfHelper
{
    static u16 ms_value;

    inline TypeOfHelper(cstrc name, u16 value)
    {
        if (ms_value == 0)
        {
            ms_value = value;
            printf("%s :: %d\n", name, value);
        }
    }

    inline static u16 Get()
    {
        DebugAssert(ms_value != 0);
        return ms_value;
    }
};

template<typename T>
u16 TypeOfHelper<T>::ms_value;

#define Reflect(T) static TypeOfHelper<T> _TOH_##T = TypeOfHelper<T>(#T, __COUNTER__);

#define TypeOf(T) TypeOfHelper<T>::Get()
