#pragma once

#include "common/int_types.h"

template<typename T>
struct alignas(8) vec2
{
    T x, y;

    inline T* begin() { return &x; }
    inline T* end() { return &x + 2; }
    inline const T* begin() const { return &x; }
    inline const T* end() const { return &x + 2; }
    inline T& operator[](i32 i) { DebugAssert((u32)i < 2u); return (&x)[i]; }
    inline const T& operator[](i32 i) const { DebugAssert((u32)i < 2u); return (&x)[i]; }
};

template<typename T>
struct vec3
{
    T x, y, z;

    inline T* begin() { return &x; }
    inline T* end() { return &x + 3; }
    inline const T* begin() const { return &x; }
    inline const T* end() const { return &x + 3; }
    inline T& operator[](i32 i) { DebugAssert((u32)i < 3u); return (&x)[i]; }
    inline const T& operator[](i32 i) const { DebugAssert((u32)i < 3u); return (&x)[i]; }
};

template<typename T>
struct alignas(16) vec4
{
    T x, y, z, w;

    inline T* begin() { return &x; }
    inline T* end() { return &x + 4; }
    inline const T* begin() const { return &x; }
    inline const T* end() const { return &x + 4; }
    inline T& operator[](i32 i) { DebugAssert((u32)i < 4u); return (&x)[i]; }
    inline const T& operator[](i32 i) const { DebugAssert((u32)i < 4u); return (&x)[i]; }
};

using float2 = vec2<f32>;
using float3 = vec3<f32>;
using float4 = vec4<f32>;

using int2 = vec2<i32>;
using int3 = vec3<i32>;
using int4 = vec4<i32>;

using uint2 = vec2<u32>;
using uint3 = vec3<u32>;
using uint4 = vec4<u32>;
