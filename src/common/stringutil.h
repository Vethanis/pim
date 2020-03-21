#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include <stdint.h>
#include "common/valist.h"

// ----------------------------------------------------------------------------
// null termination

int32_t NullTerminate(char* dst, int32_t size, int32_t length);

// ----------------------------------------------------------------------------
// case conversion

char ChrUp(char x);
char ChrLo(char x);

int32_t StrUp(char* dst, int32_t size);
int32_t StrLo(char* dst, int32_t size);

// ----------------------------------------------------------------------------
// string length

int32_t StrLen(const char* x);
int32_t StrNLen(const char* x, int32_t size);

// ----------------------------------------------------------------------------
// string copying

int32_t StrCpy(char* dst, int32_t size, const char* src);
int32_t StrCat(char* dst, int32_t size, const char* src);

int32_t ShiftRight(char* dst, int32_t size, int32_t shifts);
int32_t ShiftLeft(char* dst, int32_t size, int32_t shifts);
int32_t Shift(char* dst, int32_t size, int32_t shifts);

// ----------------------------------------------------------------------------
// string compare

int32_t StrCmp(const char* lhs, int32_t size, const char* rhs);
int32_t StrICmp(const char* lhs, int32_t size, const char* rhs);

// ----------------------------------------------------------------------------
// string searching

const char* StrChr(const char* hay, int32_t size, char needle);
const char* StrIChr(const char* hay, int32_t size, char needle);

const char* StrStr(const char* hay, int32_t size, const char* needle);
const char* StrIStr(const char* hay, int32_t size, const char* needle);

const char* StartsWith(const char* hay, int32_t size, const char* needle);
const char* IStartsWith(const char* hay, int32_t size, const char* needle);

const char* EndsWith(const char* hay, int32_t size, const char* needle);
const char* IEndsWith(const char* hay, int32_t size, const char* needle);

// ----------------------------------------------------------------------------
// string formatting

int32_t VSPrintf(char* dst, int32_t size, const char* fmt, VaList va);
int32_t SPrintf(char* dst, int32_t size, const char* fmt, ...);
int32_t StrCatf(char* dst, int32_t size, const char* fmt, ...);

// ----------------------------------------------------------------------------
// char replace

int32_t ChrRep(char* dst, int32_t size, char fnd, char rep);
int32_t ChrIRep(char* dst, int32_t size, char fnd, char rep);

// ----------------------------------------------------------------------------
// string replace

int32_t StrRep(char* dst, int32_t size, const char* fnd, const char* rep);
int32_t StrIRep(char* dst, int32_t size, const char* fnd, const char* rep);

// ----------------------------------------------------------------------------
// paths

char ChrPath(char c);
int32_t StrPath(char* dst, int32_t size);

int32_t PathCpy(char* dst, int32_t size, const char* src);
int32_t PathCat(char* dst, int32_t size, const char* src);

PIM_C_END
