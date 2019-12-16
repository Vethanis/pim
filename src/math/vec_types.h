#pragma once

#include "common/macro.h"
#include "common/int_types.h"

template<typename T>
struct alignas(sizeof(T) * 2) vec2
{
    T x, y;

    static constexpr T t0 = (T)0;
    static constexpr T t1 = (T)1;

    inline constexpr vec2() : x(t0), y(t0) {}
    inline constexpr vec2(T s) : x(s), y(s) {}
    inline constexpr vec2(T s1, T s2) : x(s1), y(s2) {}
    inline constexpr vec2(const T * const v) : x(v[0]), y(v[1]) {}

    inline constexpr T& operator[](i32 i) { ASSERT((u32)i < 2u); return (&x)[i]; }
    inline constexpr const T& operator[](i32 i) const { ASSERT((u32)i < 2u); return (&x)[i]; }
};

template<typename T>
struct vec3
{
    T x, y, z;

    static constexpr T t0 = (T)0;
    static constexpr T t1 = (T)1;

    inline constexpr vec3() : x(t0), y(t0), z(t0) {}
    inline constexpr vec3(T s) : x(s), y(s), z(s) {}
    inline constexpr vec3(T s1, T s2, T s3) : x(s1), y(s2), z(s3) {}
    inline constexpr vec3(const T * const v) : x(v[0]), y(v[1]), z(v[2]) {}

    inline constexpr vec3(vec2<T> v) : x(v.x), y(v.y), z(t0) {}
    inline constexpr vec3(vec2<T> v, T s) : x(v.x), y(v.y), z(s) {}
    inline constexpr vec3(T s, vec2<T> v) : x(s), y(v.x), z(v.y) {}

    inline constexpr T& operator[](i32 i) { ASSERT((u32)i < 3u); return (&x)[i]; }
    inline constexpr const T& operator[](i32 i) const { ASSERT((u32)i < 3u); return (&x)[i]; }
};

template<typename T>
struct alignas(sizeof(T) * 4) vec4
{
    T x, y, z, w;

    static constexpr T t0 = (T)0;
    static constexpr T t1 = (T)1;

    inline constexpr vec4() : x(t0), y(t0), z(t0), w(t0) {}
    inline constexpr vec4(T s) : x(s), y(s), z(s), w(s) {}
    inline constexpr vec4(T s1, T s2, T s3, T s4) : x(s1), y(s2), z(s3), w(s4) {}
    inline constexpr vec4(const T * const v) : x(v[0]), y(v[1]), z(v[2]), w(v[3]) {}

    inline constexpr vec4(vec2<T> v) : x(v.x), y(v.y), z(t0), w(t0) {}
    inline constexpr vec4(vec2<T> v, T s1, T s2) : x(v.x), y(v.y), z(s1), w(s2) {}
    inline constexpr vec4(T s1, vec2<T> v, T s2) : x(s1), y(v.x), z(v.y), w(s2) {}
    inline constexpr vec4(T s1, T s2, vec2<T> v) : x(s1), y(s2), z(v.x), w(v.y) {}
    inline constexpr vec4(vec2<T> v1, vec2<T> v2) : x(v1.x), y(v1.y), z(v2.x), w(v2.y) {}

    inline constexpr vec4(vec3<T> v) : x(v.x), y(v.y), z(v.z), w(t0) {}
    inline constexpr vec4(vec3<T> v, T s) : x(v.x), y(v.y), z(v.z), w(s) {}
    inline constexpr vec4(T s, vec3<T> v) : x(s), y(v.x), z(v.y), w(v.z) {}

    inline constexpr T& operator[](i32 i) { ASSERT((u32)i < 4u); return (&x)[i]; }
    inline constexpr const T& operator[](i32 i) const { ASSERT((u32)i < 4u); return (&x)[i]; }
};

using bool2 = vec2<u32>;
using bool3 = vec3<u32>;
using bool4 = vec4<u32>;

using sbyte2 = vec2<i8>;
using sbyte3 = vec3<i8>;
using sbyte4 = vec4<i8>;

using byte2 = vec2<u8>;
using byte3 = vec3<u8>;
using byte4 = vec4<u8>;

using short2 = vec2<i16>;
using short3 = vec3<i16>;
using short4 = vec4<i16>;

using ushort2 = vec2<u16>;
using ushort3 = vec3<u16>;
using ushort4 = vec4<u16>;

using int2 = vec2<i32>;
using int3 = vec3<i32>;
using int4 = vec4<i32>;

using uint2 = vec2<u32>;
using uint3 = vec3<u32>;
using uint4 = vec4<u32>;

using long2 = vec2<i64>;
using long3 = vec3<i64>;
using long4 = vec4<i64>;

using ulong2 = vec2<u64>;
using ulong3 = vec3<u64>;
using ulong4 = vec4<u64>;

using float2 = vec2<f32>;
using float3 = vec3<f32>;
using float4 = vec4<f32>;

using double2 = vec2<f64>;
using double3 = vec3<f64>;
using double4 = vec4<f64>;

struct quaternion
{
    float4 Value;
};
