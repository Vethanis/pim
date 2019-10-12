#pragma once

using usize = unsigned __int64;
using isize = signed __int64;

using i64 = signed __int64;
using i32 = signed __int32;
using i16 = signed __int16;
using i8 = signed __int8;

using u64 = unsigned __int64;
using u32 = unsigned __int32;
using u16 = unsigned __int16;
using u8 = unsigned __int8;

using f64 = double;
using f32 = float;

using cstr = const char*;
using cstrc = const char * const;

// win32's DWORD
using ul32 = unsigned long;

template<typename T>
struct Slice;

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
