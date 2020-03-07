#pragma once

#include "common/macro.h"
#include "common/int_types.h"

struct Module
{
    const char* (*GetName)();
    void(*Init)(i32 argc, const char** argv);
    void(*Update)();
    void(*Shutdown)();
};
