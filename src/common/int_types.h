#pragma once

#include <stdint.h>
#include <stddef.h>

typedef size_t usize;
typedef intptr_t isize;

typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t i8;

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef double f64;
typedef float f32;

typedef void* ptr_t;
typedef const char* cstr;
typedef const char* const cstrc;

typedef enum
{
    EUnknown = 0,
    ESuccess = 1,
    EFail = 2,
} EResult;
