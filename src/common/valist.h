#pragma once

#include "common/round.h"

// Stack pointer to the pushed arguments of a cdecl function
typedef char* VaList;

// Get a pointer to the argument stack immediately following 'arg'
#define VaStart(arg)                ( (VaList)(&(arg)) + AlignRemPtr(sizeof(arg)) )

// Advance the argument pointer by the size provided
#define VaNext(va, size)            ( ( va += AlignRemPtr(size) ) - AlignRemPtr(size) )

// Advance the argument pointer to the next function argument, and dereference it
#define VaArg(va, T)                *(T*)(VaNext(va, sizeof(T)))
