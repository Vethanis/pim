#pragma once

#include <stdint.h>
#include <stddef.h>

using usize = size_t;
using isize = ptrdiff_t;

using i64 = int64_t;
using i32 = int32_t;
using i16 = int16_t;
using i8 = int8_t;

using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u8 = uint8_t;

using f64 = double;
using f32 = float;

using cstr = const char*;
using cstrc = const char * const;

// win32's DWORD
using ul32 = unsigned long;

template<typename T>
struct Slice;

enum AllocType : u8
{
    Alloc_Stdlib = 0,
    Alloc_Linear,
    Alloc_Stack,
    Alloc_Pool,

    Alloc_COUNT
};

template<typename T>
struct Array;

template<typename T, i32 t_capacity>
struct FixedArray;

template<typename K, typename V>
struct Dict;

template<u32 t_width, typename K, typename V>
struct DictTable;

template<u32 t_capacity>
struct Ring;
