#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include <stdint.h>

// ----------------------------------------------------------------------------
// Fnv32

#define Fnv32Bias 2166136261u
#define Fnv32Prime 16777619u

static char FnvToUpper(char x)
{
    int32_t c = (int32_t)x;
    int32_t lo = ('a' - 1) - c;     // c >= 'a'
    int32_t hi = c - ('z' + 1);     // c <= 'z'
    int32_t mask = (lo & hi) >> 9;  // &&
    c = c + (mask & ('A' - 'a'));
    return (char)c;
}

static uint32_t Fnv32Char(char x, uint32_t hash)
{
    return (hash ^ FnvToUpper(x)) * Fnv32Prime;
}

static uint32_t Fnv32Byte(uint8_t x, uint32_t hash)
{
    return (hash ^ x) * Fnv32Prime;
}

static uint32_t Fnv32Word(uint16_t x, uint32_t hash)
{
    hash = (hash ^ (0xff & (x >> 0))) * Fnv32Prime;
    hash = (hash ^ (0xff & (x >> 8))) * Fnv32Prime;
    return hash;
}

static uint32_t Fnv32Dword(uint32_t x, uint32_t hash)
{
    hash = (hash ^ (0xff & (x >> 0))) * Fnv32Prime;
    hash = (hash ^ (0xff & (x >> 8))) * Fnv32Prime;
    hash = (hash ^ (0xff & (x >> 16))) * Fnv32Prime;
    hash = (hash ^ (0xff & (x >> 24))) * Fnv32Prime;
    return hash;
}

static uint32_t Fnv32Qword(uint64_t x, uint32_t hash)
{
    hash = (hash ^ (0xff & (x >> 0))) * Fnv32Prime;
    hash = (hash ^ (0xff & (x >> 8))) * Fnv32Prime;
    hash = (hash ^ (0xff & (x >> 16))) * Fnv32Prime;
    hash = (hash ^ (0xff & (x >> 24))) * Fnv32Prime;

    hash = (hash ^ (0xff & (x >> 32))) * Fnv32Prime;
    hash = (hash ^ (0xff & (x >> 40))) * Fnv32Prime;
    hash = (hash ^ (0xff & (x >> 48))) * Fnv32Prime;
    hash = (hash ^ (0xff & (x >> 56))) * Fnv32Prime;
    return hash;
}

static uint32_t Fnv32String(const char* str, uint32_t hash)
{
    ASSERT(str);
    for (int32_t i = 0; str[i]; ++i)
    {
        hash = Fnv32Char(str[i], hash);
    }
    return hash;
}

static uint32_t Fnv32Bytes(const void* ptr, int32_t nBytes, uint32_t hash)
{
    ASSERT(ptr || !nBytes);
    ASSERT(nBytes >= 0);
    const uint8_t* asBytes = (const uint8_t*)ptr;
    for (int32_t i = 0; i < nBytes; ++i)
    {
        hash = Fnv32Byte(asBytes[i], hash);
    }
    return hash;
}

// ----------------------------------------------------------------------------
// Fnv64

#define Fnv64Bias 14695981039346656037ull
#define Fnv64Prime 1099511628211ull

static uint64_t Fnv64Char(char x, uint64_t hash)
{
    return (hash ^ FnvToUpper(x)) * Fnv64Prime;
}

static uint64_t Fnv64Byte(uint8_t x, uint64_t hash)
{
    return (hash ^ x) * Fnv64Prime;
}

static uint64_t Fnv64Word(uint16_t x, uint64_t hash)
{
    hash = (hash ^ (0xff & (x >> 0))) * Fnv64Prime;
    hash = (hash ^ (0xff & (x >> 8))) * Fnv64Prime;
    return hash;
}

static uint64_t Fnv64Dword(uint32_t x, uint64_t hash)
{
    hash = (hash ^ (0xff & (x >> 0))) * Fnv64Prime;
    hash = (hash ^ (0xff & (x >> 8))) * Fnv64Prime;
    hash = (hash ^ (0xff & (x >> 16))) * Fnv64Prime;
    hash = (hash ^ (0xff & (x >> 24))) * Fnv64Prime;
    return hash;
}

static uint64_t Fnv64Qword(uint64_t x, uint64_t hash)
{
    hash = (hash ^ (0xff & (x >> 0))) * Fnv64Prime;
    hash = (hash ^ (0xff & (x >> 8))) * Fnv64Prime;
    hash = (hash ^ (0xff & (x >> 16))) * Fnv64Prime;
    hash = (hash ^ (0xff & (x >> 24))) * Fnv64Prime;
    hash = (hash ^ (0xff & (x >> 32))) * Fnv64Prime;
    hash = (hash ^ (0xff & (x >> 40))) * Fnv64Prime;
    hash = (hash ^ (0xff & (x >> 48))) * Fnv64Prime;
    hash = (hash ^ (0xff & (x >> 56))) * Fnv64Prime;
    return hash;
}

static uint64_t Fnv64String(const char* ptr, uint64_t hash)
{
    ASSERT(ptr);
    for (int32_t i = 0; ptr[i]; ++i)
    {
        hash = Fnv64Char(ptr[i], hash);
    }
    return hash;
}

static uint64_t Fnv64Bytes(const void* ptr, int32_t nBytes, uint64_t hash)
{
    ASSERT(ptr || !nBytes);
    ASSERT(nBytes >= 0);
    const uint8_t* asBytes = (const uint8_t*)ptr;
    for (int32_t i = 0; i < nBytes; ++i)
    {
        hash = Fnv64Byte(asBytes[i], hash);
    }
    return hash;
}

PIM_C_END
