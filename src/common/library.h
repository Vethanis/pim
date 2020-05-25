#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct library_s
{
    void* handle;
} library_t;

library_t Library_Open(const char* name);
void Library_Close(library_t lib);
void* Library_Sym(library_t lib, const char* name);

PIM_C_END
