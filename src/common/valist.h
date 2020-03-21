#pragma once

#include "common/round.h"

// Stack pointer to the pushed arguments of a cdecl function
typedef char* VaList;

// Get a pointer to the argument stack immediately following 'arg'
#define VA_START(arg)                ( (VaList)(&(arg)) + ALIGN_REM_PTR(sizeof(arg)) )

// Advance the argument pointer by the size provided
#define VA_NEXT(va, size)            ( ( va += ALIGN_REM_PTR(size) ) - ALIGN_REM_PTR(size) )

// Advance the argument pointer to the next function argument, and dereference it
#define VA_ARG(va, T)                ( *(T*)(VA_NEXT(va, sizeof(T))) )
