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
