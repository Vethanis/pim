#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "common/valist.h"

// ----------------------------------------------------------------------------
// null termination

i32 NullTerminate(char* dst, i32 size, i32 length);

// ----------------------------------------------------------------------------
// case conversion

char ChrUp(char x);
char ChrLo(char x);

i32 StrUp(char* dst, i32 size);
i32 StrLo(char* dst, i32 size);

bool IsAlpha(char c); // alphabet chars test
bool IsDigit(char c); // digit chars test
bool IsSpace(char c); // whitespace chars test

// ----------------------------------------------------------------------------
// string length

i32 StrLen(const char* x);
i32 StrNLen(const char* x, i32 size);

// ----------------------------------------------------------------------------
// string copying

i32 StrCpy(char* dst, i32 size, const char* src);
i32 StrCat(char* dst, i32 size, const char* src);

char* StrDup(const char* src, EAlloc allocator);

i32 ShiftRight(char* dst, i32 size, i32 shifts);
i32 ShiftLeft(char* dst, i32 size, i32 shifts);
i32 Shift(char* dst, i32 size, i32 shifts);

// ----------------------------------------------------------------------------
// string compare

i32 StrCmp(const char* lhs, i32 size, const char* rhs);
i32 StrICmp(const char* lhs, i32 size, const char* rhs);
i32 MemICmp(const char* lhs, const char* rhs, i32 size);

// ----------------------------------------------------------------------------
// string searching

const char* StrChr(const char* hay, i32 size, char needle);
const char* StrIChr(const char* hay, i32 size, char needle);

const char* StrStr(const char* hay, i32 size, const char* needle);
const char* StrIStr(const char* hay, i32 size, const char* needle);

const char* StartsWith(const char* hay, i32 size, const char* needle);
const char* IStartsWith(const char* hay, i32 size, const char* needle);

const char* EndsWith(const char* hay, i32 size, const char* needle);
const char* IEndsWith(const char* hay, i32 size, const char* needle);

// ----------------------------------------------------------------------------
// string formatting

i32 VSPrintf(char* dst, i32 size, const char* fmt, VaList va);
i32 SPrintf(char* dst, i32 size, const char* fmt, ...);
i32 StrCatf(char* dst, i32 size, const char* fmt, ...);

// ----------------------------------------------------------------------------
// char replace

i32 ChrRep(char* dst, i32 size, char fnd, char rep);
i32 ChrIRep(char* dst, i32 size, char fnd, char rep);

// ----------------------------------------------------------------------------
// string replace

i32 StrRep(char* dst, i32 size, const char* fnd, const char* rep);
i32 StrIRep(char* dst, i32 size, const char* fnd, const char* rep);

// ----------------------------------------------------------------------------
// paths

char ChrPath(char c);
i32 StrPath(char* dst, i32 size);

i32 PathCpy(char* dst, i32 size, const char* src);
i32 PathCat(char* dst, i32 size, const char* src);

PIM_C_END
