#pragma once

#include "common/int_types.h"

// ----------------------------------------------------------------------------
// null termination

i32 NullTerminate(char* dst, i32 size, i32 length);

// ----------------------------------------------------------------------------
// case conversion

char ToUpper(char x);
char ToLower(char x);

i32 ToLower(char* dst, i32 size);
i32 ToUpper(char* dst, i32 size);

// ----------------------------------------------------------------------------
// string length

i32 StrLen(cstrc x);
i32 StrLen(cstrc x, i32 size);

// ----------------------------------------------------------------------------
// string copying

i32 StrCpy(char* dst, i32 size, cstr src);
i32 StrCat(char* dst, i32 size, cstr src);

i32 ShiftRight(char* dst, i32 size, i32 shifts);
i32 ShiftLeft(char* dst, i32 size, i32 shifts);
i32 Shift(char* dst, i32 size, i32 shifts);

// ----------------------------------------------------------------------------
// string compare

i32 StrCmp(cstrc lhs, i32 size, cstrc rhs);

i32 StrICmp(cstrc lhs, i32 size, cstrc rhs);

// ----------------------------------------------------------------------------
// string searching

cstr StrChr(cstr hay, i32 size, char needle);
cstr StrIChr(cstr hay, i32 size, char needle);

cstr StrStr(cstr hay, i32 size, cstr needle);
cstr StrIStr(cstr hay, i32 size, cstr needle);

cstr StartsWith(cstr hay, i32 size, cstr needle);
cstr IStartsWith(cstr hay, i32 size, cstr needle);
cstr StartsWith(cstr hay, i32 size, char needle);
cstr IStartsWith(cstr hay, i32 size, char needle);

cstr EndsWith(cstr hay, i32 size, cstr needle);
cstr IEndsWith(cstr hay, i32 size, cstr needle);
cstr EndsWith(cstr hay, i32 size, char needle);
cstr IEndsWith(cstr hay, i32 size, char needle);

// ----------------------------------------------------------------------------
// string formatting

i32 SPrintf(char* dst, i32 size, cstr fmt, ...);
i32 StrCatf(char* dst, i32 size, cstr fmt, ...);

// ----------------------------------------------------------------------------
// char replace

i32 ChrRep(char* dst, i32 size, char fnd, char rep);
i32 ChrIRep(char* dst, i32 size, char fnd, char rep);

// ----------------------------------------------------------------------------
// string replace

i32 StrRep(char* dst, i32 size, cstr fnd, cstr rep);
i32 StrIRep(char* dst, i32 size, cstr fnd, cstr rep);

// ----------------------------------------------------------------------------
// paths

char FixPath(char c);
i32 FixPath(char* dst, i32 size);

i32 PathCpy(char* dst, i32 size, cstr src);
i32 PathCat(char* dst, i32 size, cstr src);
