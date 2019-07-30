#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef size_t usize;
typedef ptrdiff_t isize;

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

typedef const char* cstr;

// https://en.cppreference.com/w/c/language/_Alignas

typedef struct float2 { _Alignas( 8) f32 x, y; } float2;
typedef struct float3 { _Alignas( 4) f32 x, y, z; } float3;
typedef struct float4 { _Alignas(16) f32 x, y, z, w; } float4;

typedef struct int2 { _Alignas( 8) i32 x, y; } int2;
typedef struct int3 { _Alignas( 4) i32 x, y, z; } int3;
typedef struct int4 { _Alignas(16) i32 x, y, z, w; } int4;

typedef struct uint2 { _Alignas( 8) u32 x, y; } uint2;
typedef struct uint3 { _Alignas( 4) u32 x, y, z; } uint3;
typedef struct uint4 { _Alignas(16) u32 x, y, z, w; } uint4;

typedef struct float2x2 { float2 x, y; } float2x2;
typedef struct float3x3 { float3 x, y, z; } float3x3;
typedef struct float4x4 { float4 x, y, z, w; } float4x4;

typedef struct int2x2 { int2 x, y; } int2x2;
typedef struct int3x3 { int3 x, y, z; } int3x3;
typedef struct int4x4 { int4 x, y, z, w; } int4x4;

typedef struct uint2x2 { uint2 x, y; } uint2x2;
typedef struct uint3x3 { uint3 x, y, z; } uint3x3;
typedef struct uint4x4 { uint4 x, y, z, w; } uint4x4;

#define Slice(name, T) typedef struct name { T* ptr; i32 len; } name

Slice(Bytes, u8);
Slice(CBytes, const u8);
Slice(Dwords, u32);
Slice(CDwords, const u32);
Slice(Qwords, u64);
Slice(CQwords, const u64);
Slice(Floats, f32);
Slice(CFloats, const f32);
Slice(Chars, char);
Slice(CChars, const char);

#define Array(name, T) typedef struct name { T* ptr; i32 len; i32 cap; } name

Array(ByteArray, u8);
Array(I32Array, i32);
Array(U32Array, u32);
Array(U64Array, u64);
Array(F32Array, f32);
Array(CharArray, char);
