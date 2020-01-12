#pragma once

#include "common/int_types.h"

struct Task
{
    void(*Run)(Task& task);
    u64 buffer[7];
};

namespace TaskSystem
{
    void Schedule(const Task& task);
};
