#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct Library_s
{
    void* handle;
} Library;

bool Library_IsOpen(Library l);
Library Library_Open(const char* name);
void Library_Close(Library lib);
void* Library_Sym(Library lib, const char* name);

PIM_C_END
