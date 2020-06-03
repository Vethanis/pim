#pragma once

// Divide x by y while rounding up
#define DIV_ROUND(x, y)         ( ((x) + (y) - 1) / (y) )

// Scale x to a multiple of y
#define MULTIPLE_OF(x, y)       ( DIV_ROUND(x, y) * (y) )

// Returns the remainder of x when divided by y
// Assumes that y is a power of 2
#define ALIGN_REM(x, y)         ( ((x) + (y) - 1) & (~((y) - 1)) )

// Returns the remainder of x when divided by the address size
#define ALIGN_REM_PTR(x)        ALIGN_REM(x, sizeof(void*))

// Stack pointer to the pushed arguments of a cdecl function
typedef char* VaList;

// Get a pointer to the argument stack immediately following 'arg'
#define VA_START(arg)                ( (VaList)(&(arg)) + ALIGN_REM_PTR(sizeof(arg)) )

// Advance the argument pointer by the size provided
#define VA_NEXT(va, size)            ( ( va += ALIGN_REM_PTR(size) ) - ALIGN_REM_PTR(size) )

// Advance the argument pointer to the next function argument, and dereference it
#define VA_ARG(va, T)                ( *(T*)(VA_NEXT(va, sizeof(T))) )
