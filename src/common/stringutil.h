#pragma once

#include "common/macro.h"

PIM_C_BEGIN

// ----------------------------------------------------------------------------
// null termination

i32 NullTerminate(char *const dst, i32 size, i32 length);

// ----------------------------------------------------------------------------
// case conversion

char ChrUp(char x);
char ChrLo(char x);

i32 StrUp(char *const dst, i32 size);
i32 StrLo(char *const dst, i32 size);

bool IsAlpha(char c); // alphabet chars test
bool IsDigit(char c); // digit chars test
bool IsSpace(char c); // whitespace chars test

// ----------------------------------------------------------------------------
// string length

i32 StrLen(char const *const x);
i32 StrNLen(char const *const x, i32 size);

// ----------------------------------------------------------------------------
// string copying

i32 StrCpy(char *const dst, i32 size, char const *const src);
i32 StrCat(char *const dst, i32 size, char const *const src);

char* StrDup(char const *const src, EAlloc allocator);

i32 ShiftRight(char *const dst, i32 size, i32 shifts);
i32 ShiftLeft(char *const dst, i32 size, i32 shifts);
i32 Shift(char *const dst, i32 size, i32 shifts);

// ----------------------------------------------------------------------------
// string compare

i32 StrCmp(char const *const lhs, i32 size, char const *const rhs);
i32 StrICmp(char const *const lhs, i32 size, char const *const rhs);
i32 MemICmp(char const *const pim_noalias lhs, char const *const pim_noalias rhs, i32 size);

// ----------------------------------------------------------------------------
// string searching

const char* StrChr(char const *const hay, i32 size, char needle);
const char* StrIChr(char const *const hay, i32 size, char needle);

const char* StrRChr(char const *const hay, i32 size, char needle);
const char* StrRIChr(char const *const hay, i32 size, char needle);

const char* StrStr(char const *const hay, i32 size, char const *const needle);
const char* StrIStr(char const *const hay, i32 size, char const *const needle);

const char* StartsWith(char const *const hay, i32 size, char const *const needle);
const char* IStartsWith(char const *const hay, i32 size, char const *const needle);

const char* EndsWith(char const *const hay, i32 size, char const *const needle);
const char* IEndsWith(char const *const hay, i32 size, char const *const needle);

// ----------------------------------------------------------------------------
// string formatting

i32 VSPrintf(char *const dst, i32 size, char const *const fmt, va_list va);
i32 SPrintf(char *const dst, i32 size, char const *const fmt, ...);
i32 VStrCatf(char *const dst, i32 size, char const *const fmt, va_list va);
i32 StrCatf(char *const dst, i32 size, char const *const fmt, ...);

// ----------------------------------------------------------------------------
// char replace

i32 ChrRep(char *const dst, i32 size, char fnd, char rep);
i32 ChrIRep(char *const dst, i32 size, char fnd, char rep);

// ----------------------------------------------------------------------------
// string replace

i32 StrRep(char *const dst, i32 size, char const *const fnd, char const *const rep);
i32 StrIRep(char *const dst, i32 size, char const *const fnd, char const *const rep);

// ----------------------------------------------------------------------------
// paths

bool IsPathSeparator(char c);
char ChrPath(char c);
i32 StrPath(char *const dst, i32 size);

// ----------------------------------------------------------------------------
// parsing

i32 ParseInt(const char* x);
float ParseFloat(const char* x);

PIM_C_END
