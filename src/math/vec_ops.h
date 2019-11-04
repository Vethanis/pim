#pragma once

#include "math/vec_types.h"

// ----------------------------------------------------------------------------

template<typename T>
inline vec2<T>& operator - (vec2<T>& x)
{
    x.x = -x.x;
    x.y = -x.y;
    return x;
}
template<typename T>
inline vec3<T>& operator - (vec3<T>& x)
{
    x.x = -x.x;
    x.y = -x.y;
    x.z = -x.z;
    return x;
}
template<typename T>
inline vec4<T>& operator - (vec4<T>& x)
{
    x.x = -x.x;
    x.y = -x.y;
    x.z = -x.z;
    x.w = -x.w;
    return x;
}

// ----------------------------------------------------------------------------

template<typename T>
inline vec2<T> operator + (vec2<T> a, vec2<T> b)
{
    return { a.x + b.x, a.y + b.y };
}
template<typename T>
inline vec2<T> operator - (vec2<T> a, vec2<T> b)
{
    return { a.x - b.x, a.y - b.y };
}
template<typename T>
inline vec2<T> operator * (vec2<T> a, vec2<T> b)
{
    return { a.x * b.x, a.y * b.y };
}
template<typename T>
inline vec2<T> operator / (vec2<T> a, vec2<T> b)
{
    return { a.x / b.x, a.y / b.y };
}
template<typename T>
inline vec2<T> operator % (vec2<T> a, vec2<T> b)
{
    return { a.x % b.x, a.y % b.y };
}

template<typename T>
inline vec2<T> operator + (T a, vec2<T> b)
{
    return { a + b.x, a + b.y };
}
template<typename T>
inline vec2<T> operator - (T a, vec2<T> b)
{
    return { a - b.x, a - b.y };
}
template<typename T>
inline vec2<T> operator * (T a, vec2<T> b)
{
    return { a * b.x, a * b.y };
}
template<typename T>
inline vec2<T> operator / (T a, vec2<T> b)
{
    return { a / b.x, a / b.y };
}
template<typename T>
inline vec2<T> operator % (T a, vec2<T> b)
{
    return { a % b.x, a % b.y };
}

template<typename T>
inline vec2<T> operator + (vec2<T> a, T b)
{
    return { a.x + b, a.y + b };
}
template<typename T>
inline vec2<T> operator - (vec2<T> a, T b)
{
    return { a.x - b, a.y - b };
}
template<typename T>
inline vec2<T> operator * (vec2<T> a, T b)
{
    return { a.x * b, a.y * b };
}
template<typename T>
inline vec2<T> operator / (vec2<T> a, T b)
{
    return { a.x / b, a.y / b };
}
template<typename T>
inline vec2<T> operator % (vec2<T> a, T b)
{
    return { a.x % b, a.y % b };
}

// ----------------------------------------------------------------------------

template<typename T>
inline vec3<T> operator + (vec3<T> a, vec3<T> b)
{
    return { a.x + b.x, a.y + b.y, a.z + b.z };
}
template<typename T>
inline vec3<T> operator - (vec3<T> a, vec3<T> b)
{
    return { a.x - b.x, a.y - b.y, a.z - b.z };
}
template<typename T>
inline vec3<T> operator * (vec3<T> a, vec3<T> b)
{
    return { a.x * b.x, a.y * b.y, a.z * b.z };
}
template<typename T>
inline vec3<T> operator / (vec3<T> a, vec3<T> b)
{
    return { a.x / b.x, a.y / b.y, a.z / b.z };
}
template<typename T>
inline vec3<T> operator % (vec3<T> a, vec3<T> b)
{
    return { a.x % b.x, a.y % b.y, a.z % b.z };
}

template<typename T>
inline vec3<T> operator + (T a, vec3<T> b)
{
    return { a + b.x, a + b.y, a + b.z };
}
template<typename T>
inline vec3<T> operator - (T a, vec3<T> b)
{
    return { a - b.x, a - b.y, a - b.z };
}
template<typename T>
inline vec3<T> operator * (T a, vec3<T> b)
{
    return { a * b.x, a * b.y, a * b.z };
}
template<typename T>
inline vec3<T> operator / (T a, vec3<T> b)
{
    return { a / b.x, a / b.y, a / b.z };
}
template<typename T>
inline vec3<T> operator % (T a, vec3<T> b)
{
    return { a % b.x, a % b.y, a % b.z };
}

template<typename T>
inline vec3<T> operator + (vec3<T> a, T b)
{
    return { a.x + b, a.y + b, a.z + b };
}
template<typename T>
inline vec3<T> operator - (vec3<T> a, T b)
{
    return { a.x - b, a.y - b, a.z - b };
}
template<typename T>
inline vec3<T> operator * (vec3<T> a, T b)
{
    return { a.x * b, a.y * b, a.z * b };
}
template<typename T>
inline vec3<T> operator / (vec3<T> a, T b)
{
    return { a.x / b, a.y / b, a.z / b };
}
template<typename T>
inline vec3<T> operator % (vec3<T> a, T b)
{
    return { a.x % b, a.y % b, a.z % b };
}

// ----------------------------------------------------------------------------

template<typename T>
inline vec4<T> operator + (vec4<T> a, vec4<T> b)
{
    return { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
}
template<typename T>
inline vec4<T> operator - (vec4<T> a, vec4<T> b)
{
    return { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
}
template<typename T>
inline vec4<T> operator * (vec4<T> a, vec4<T> b)
{
    return { a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w };
}
template<typename T>
inline vec4<T> operator / (vec4<T> a, vec4<T> b)
{
    return { a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w };
}
template<typename T>
inline vec4<T> operator % (vec4<T> a, vec4<T> b)
{
    return { a.x % b.x, a.y % b.y, a.z % b.z, a.w % b.w };
}

template<typename T>
inline vec4<T> operator + (T a, vec4<T> b)
{
    return { a + b.x, a + b.y, a + b.z, a + b.w };
}
template<typename T>
inline vec4<T> operator - (T a, vec4<T> b)
{
    return { a - b.x, a - b.y, a - b.z, a - b.w };
}
template<typename T>
inline vec4<T> operator * (T a, vec4<T> b)
{
    return { a * b.x, a * b.y, a * b.z, a * b.w };
}
template<typename T>
inline vec4<T> operator / (T a, vec4<T> b)
{
    return { a / b.x, a / b.y, a / b.z, a / b.w };
}
template<typename T>
inline vec4<T> operator % (T a, vec4<T> b)
{
    return { a % b.x, a % b.y, a % b.z, a % b.w };
}

template<typename T>
inline vec4<T> operator + (vec4<T> a, T b)
{
    return { a.x + b, a.y + b, a.z + b, a.w + b };
}
template<typename T>
inline vec4<T> operator - (vec4<T> a, T b)
{
    return { a.x - b, a.y - b, a.z - b, a.w - b };
}
template<typename T>
inline vec4<T> operator * (vec4<T> a, T b)
{
    return { a.x * b, a.y * b, a.z * b, a.w * b };
}
template<typename T>
inline vec4<T> operator / (vec4<T> a, T b)
{
    return { a.x / b, a.y / b, a.z / b, a.w / b };
}
template<typename T>
inline vec4<T> operator % (vec4<T> a, T b)
{
    return { a.x % b, a.y % b, a.z % b, a.w % b };
}

// ----------------------------------------------------------------------------

template<typename T>
inline vec2<T>& operator += (vec2<T>& a, vec2<T> b)
{
    a = a + b;
    return a;
}
template<typename T>
inline vec2<T>& operator -= (vec2<T>& a, vec2<T> b)
{
    a = a - b;
    return a;
}
template<typename T>
inline vec2<T>& operator *= (vec2<T>& a, vec2<T> b)
{
    a = a * b;
    return a;
}
template<typename T>
inline vec2<T>& operator /= (vec2<T>& a, vec2<T> b)
{
    a = a / b;
    return a;
}
template<typename T>
inline vec2<T>& operator %= (vec2<T>& a, vec2<T> b)
{
    a = a % b;
    return a;
}

template<typename T>
inline vec2<T>& operator += (vec2<T>& a, T b)
{
    a = a + b;
    return a;
}
template<typename T>
inline vec2<T>& operator -= (vec2<T>& a, T b)
{
    a = a - b;
    return a;
}
template<typename T>
inline vec2<T>& operator *= (vec2<T>& a, T b)
{
    a = a * b;
    return a;
}
template<typename T>
inline vec2<T>& operator /= (vec2<T>& a, T b)
{
    a = a / b;
    return a;
}
template<typename T>
inline vec2<T>& operator %= (vec2<T>& a, T b)
{
    a = a % b;
    return a;
}

// ----------------------------------------------------------------------------

template<typename T>
inline vec3<T>& operator += (vec3<T>& a, vec3<T> b)
{
    a = a + b;
    return a;
}
template<typename T>
inline vec3<T>& operator -= (vec3<T>& a, vec3<T> b)
{
    a = a - b;
    return a;
}
template<typename T>
inline vec3<T>& operator *= (vec3<T>& a, vec3<T> b)
{
    a = a * b;
    return a;
}
template<typename T>
inline vec3<T>& operator /= (vec3<T>& a, vec3<T> b)
{
    a = a / b;
    return a;
}
template<typename T>
inline vec3<T>& operator %= (vec3<T>& a, vec3<T> b)
{
    a = a % b;
    return a;
}

template<typename T>
inline vec3<T>& operator += (vec3<T>& a, T b)
{
    a = a + b;
    return a;
}
template<typename T>
inline vec3<T>& operator -= (vec3<T>& a, T b)
{
    a = a - b;
    return a;
}
template<typename T>
inline vec3<T>& operator *= (vec3<T>& a, T b)
{
    a = a * b;
    return a;
}
template<typename T>
inline vec3<T>& operator /= (vec3<T>& a, T b)
{
    a = a / b;
    return a;
}
template<typename T>
inline vec3<T>& operator %= (vec3<T>& a, T b)
{
    a = a % b;
    return a;
}

// ----------------------------------------------------------------------------

template<typename T>
inline vec4<T>& operator += (vec4<T>& a, vec4<T> b)
{
    a = a + b;
    return a;
}
template<typename T>
inline vec4<T>& operator -= (vec4<T>& a, vec4<T> b)
{
    a = a - b;
    return a;
}
template<typename T>
inline vec4<T>& operator *= (vec4<T>& a, vec4<T> b)
{
    a = a * b;
    return a;
}
template<typename T>
inline vec4<T>& operator /= (vec4<T>& a, vec4<T> b)
{
    a = a / b;
    return a;
}
template<typename T>
inline vec4<T>& operator %= (vec4<T>& a, vec4<T> b)
{
    a = a % b;
    return a;
}

template<typename T>
inline vec4<T>& operator += (vec4<T>& a, T b)
{
    a = a + b;
    return a;
}
template<typename T>
inline vec4<T>& operator -= (vec4<T>& a, T b)
{
    a = a - b;
    return a;
}
template<typename T>
inline vec4<T>& operator *= (vec4<T>& a, T b)
{
    a = a * b;
    return a;
}
template<typename T>
inline vec4<T>& operator /= (vec4<T>& a, T b)
{
    a = a / b;
    return a;
}
template<typename T>
inline vec4<T>& operator %= (vec4<T>& a, T b)
{
    a = a % b;
    return a;
}

// ----------------------------------------------------------------------------

template<typename T>
inline bool2 operator == (vec2<T> a, vec2<T> b)
{
    return { a.x == b.x, a.y == b.y };
}
template<typename T>
inline bool2 operator == (T a, vec2<T> b)
{
    return { a == b.x, a == b.y };
}
template<typename T>
inline bool2 operator == (vec2<T> a, T b)
{
    return { a.x == b, a.y == b };
}

template<typename T>
inline bool2 operator != (vec2<T> a, vec2<T> b)
{
    return { a.x != b.x, a.y != b.y };
}
template<typename T>
inline bool2 operator != (T a, vec2<T> b)
{
    return { a != b.x, a != b.y };
}
template<typename T>
inline bool2 operator != (vec2<T> a, T b)
{
    return { a.x != b, a.y != b };
}

template<typename T>
inline bool2 operator > (vec2<T> a, vec2<T> b)
{
    return { a.x > b.x, a.y > b.y };
}
template<typename T>
inline bool2 operator > (T a, vec2<T> b)
{
    return { a > b.x, a > b.y };
}
template<typename T>
inline bool2 operator > (vec2<T> a, T b)
{
    return { a.x > b, a.y > b };
}

template<typename T>
inline bool2 operator < (vec2<T> a, vec2<T> b)
{
    return { a.x < b.x, a.y < b.y };
}
template<typename T>
inline bool2 operator < (T a, vec2<T> b)
{
    return { a < b.x, a < b.y };
}
template<typename T>
inline bool2 operator < (vec2<T> a, T b)
{
    return { a.x < b, a.y < b };
}

template<typename T>
inline bool2 operator >= (vec2<T> a, vec2<T> b)
{
    return { a.x >= b.x, a.y >= b.y };
}
template<typename T>
inline bool2 operator >= (T a, vec2<T> b)
{
    return { a >= b.x, a >= b.y };
}
template<typename T>
inline bool2 operator >= (vec2<T> a, T b)
{
    return { a.x >= b, a.y >= b };
}

template<typename T>
inline bool2 operator <= (vec2<T> a, vec2<T> b)
{
    return { a.x <= b.x, a.y <= b.y };
}
template<typename T>
inline bool2 operator <= (T a, vec2<T> b)
{
    return { a <= b.x, a <= b.y };
}
template<typename T>
inline bool2 operator <= (vec2<T> a, T b)
{
    return { a.x <= b, a.y <= b };
}

// ----------------------------------------------------------------------------

template<typename T>
inline bool3 operator == (vec3<T> a, vec3<T> b)
{
    return { a.x == b.x, a.y == b.y, a.z == b.z };
}
template<typename T>
inline bool3 operator == (T a, vec3<T> b)
{
    return { a == b.x, a == b.y, a == b.z };
}
template<typename T>
inline bool3 operator == (vec3<T> a, T b)
{
    return { a.x == b, a.y == b, a.z == b };
}

template<typename T>
inline bool3 operator != (vec3<T> a, vec3<T> b)
{
    return { a.x != b.x, a.y != b.y, a.z != b.z };
}
template<typename T>
inline bool3 operator != (T a, vec3<T> b)
{
    return { a != b.x, a != b.y, a != b.z };
}
template<typename T>
inline bool3 operator != (vec3<T> a, T b)
{
    return { a.x != b, a.y != b, a.z != b };
}

template<typename T>
inline bool3 operator > (vec3<T> a, vec3<T> b)
{
    return { a.x > b.x, a.y > b.y, a.z > b.z };
}
template<typename T>
inline bool3 operator > (T a, vec3<T> b)
{
    return { a > b.x, a > b.y, a > b.z };
}
template<typename T>
inline bool3 operator > (vec3<T> a, T b)
{
    return { a.x > b, a.y > b, a.z > b };
}

template<typename T>
inline bool3 operator < (vec3<T> a, vec3<T> b)
{
    return { a.x < b.x, a.y < b.y, a.z < b.z };
}
template<typename T>
inline bool3 operator < (T a, vec3<T> b)
{
    return { a < b.x, a < b.y, a < b.z };
}
template<typename T>
inline bool3 operator < (vec3<T> a, T b)
{
    return { a.x < b, a.y < b, a.z < b };
}

template<typename T>
inline bool3 operator >= (vec3<T> a, vec3<T> b)
{
    return { a.x >= b.x, a.y >= b.y, a.z >= b.z };
}
template<typename T>
inline bool3 operator >= (T a, vec3<T> b)
{
    return { a >= b.x, a >= b.y, a >= b.z };
}
template<typename T>
inline bool3 operator >= (vec3<T> a, T b)
{
    return { a.x >= b, a.y >= b, a.z >= b };
}

template<typename T>
inline bool3 operator <= (vec3<T> a, vec3<T> b)
{
    return { a.x <= b.x, a.y <= b.y, a.z <= b.z };
}
template<typename T>
inline bool3 operator <= (T a, vec3<T> b)
{
    return { a <= b.x, a <= b.y, a <= b.z };
}
template<typename T>
inline bool3 operator <= (vec3<T> a, T b)
{
    return { a.x <= b, a.y <= b, a.z <= b };
}

// ----------------------------------------------------------------------------

template<typename T>
inline bool4 operator == (vec4<T> a, vec4<T> b)
{
    return { a.x == b.x, a.y == b.y, a.z == b.z, a.w == b.w };
}
template<typename T>
inline bool4 operator == (T a, vec4<T> b)
{
    return { a == b.x, a == b.y, a == b.z, a == b.w };
}
template<typename T>
inline bool4 operator == (vec4<T> a, T b)
{
    return { a.x == b, a.y == b, a.z == b, a.w == b };
}

template<typename T>
inline bool4 operator != (vec4<T> a, vec4<T> b)
{
    return { a.x != b.x, a.y != b.y, a.z != b.z, a.w != b.w };
}
template<typename T>
inline bool4 operator != (T a, vec4<T> b)
{
    return { a != b.x, a != b.y, a != b.z, a != b.w };
}
template<typename T>
inline bool4 operator != (vec4<T> a, T b)
{
    return { a.x != b, a.y != b, a.z != b, a.w != b };
}

template<typename T>
inline bool4 operator > (vec4<T> a, vec4<T> b)
{
    return { a.x > b.x, a.y > b.y, a.z > b.z, a.w > b.w };
}
template<typename T>
inline bool4 operator > (T a, vec4<T> b)
{
    return { a > b.x, a > b.y, a > b.z, a > b.w };
}
template<typename T>
inline bool4 operator > (vec4<T> a, T b)
{
    return { a.x > b, a.y > b, a.z > b, a.w > b };
}

template<typename T>
inline bool4 operator < (vec4<T> a, vec4<T> b)
{
    return { a.x < b.x, a.y < b.y, a.z < b.z, a.w < b.w };
}
template<typename T>
inline bool4 operator < (T a, vec4<T> b)
{
    return { a < b.x, a < b.y, a < b.z, a < b.w };
}
template<typename T>
inline bool4 operator < (vec4<T> a, T b)
{
    return { a.x < b, a.y < b, a.z < b, a.w < b };
}

template<typename T>
inline bool4 operator >= (vec4<T> a, vec4<T> b)
{
    return { a.x >= b.x, a.y >= b.y, a.z >= b.z, a.w >= b.w };
}
template<typename T>
inline bool4 operator >= (T a, vec4<T> b)
{
    return { a >= b.x, a >= b.y, a >= b.z, a >= b.w };
}
template<typename T>
inline bool4 operator >= (vec4<T> a, T b)
{
    return { a.x >= b, a.y >= b, a.z >= b, a.w >= b };
}

template<typename T>
inline bool4 operator <= (vec4<T> a, vec4<T> b)
{
    return { a.x <= b.x, a.y <= b.y, a.z <= b.z, a.w <= b.w };
}
template<typename T>
inline bool4 operator <= (T a, vec4<T> b)
{
    return { a <= b.x, a <= b.y, a <= b.z, a <= b.w };
}
template<typename T>
inline bool4 operator <= (vec4<T> a, T b)
{
    return { a.x <= b, a.y <= b, a.z <= b, a.w <= b };
}

// ----------------------------------------------------------------------------

inline bool2 operator && (bool2 a, bool2 b)
{
    return { a.x & b.x, a.y & b.y };
}
inline bool3 operator && (bool3 a, bool3 b)
{
    return { a.x & b.x, a.y & b.y, a.z & b.z };
}
inline bool4 operator && (bool4 a, bool4 b)
{
    return { a.x & b.x, a.y & b.y, a.z & b.z, a.w & b.w };
}

inline bool2 operator || (bool2 a, bool2 b)
{
    return { a.x | b.x, a.y | b.y };
}
inline bool3 operator || (bool3 a, bool3 b)
{
    return { a.x | b.x, a.y | b.y, a.z | b.z };
}
inline bool4 operator || (bool4 a, bool4 b)
{
    return { a.x | b.x, a.y | b.y, a.z | b.z, a.w | b.w };
}

inline bool2 operator ! (bool2 a)
{
    return { ~a.x, ~a.y };
}
inline bool3 operator ! (bool3 a)
{
    return { ~a.x, ~a.y, ~a.z };
}
inline bool4 operator ! (bool4 a)
{
    return { ~a.x, ~a.y, ~a.z, ~a.w };
}

// ----------------------------------------------------------------------------

inline uint2 operator << (uint2 x, u32 n)
{
    return { x.x << n, x.y << n };
}
inline uint3 operator << (uint3 x, u32 n)
{
    return { x.x << n, x.y << n, x.z << n };
}
inline uint4 operator << (uint4 x, u32 n)
{
    return { x.x << n, x.y << n, x.z << n, x.w << n };
}
inline ulong2 operator << (ulong2 x, u32 n)
{
    return { x.x << n, x.y << n };
}
inline ulong3 operator << (ulong3 x, u32 n)
{
    return { x.x << n, x.y << n, x.z << n };
}
inline ulong4 operator << (ulong4 x, u32 n)
{
    return { x.x << n, x.y << n, x.z << n, x.w << n };
}

inline uint2 operator >> (uint2 x, u32 n)
{
    return { x.x >> n, x.y >> n };
}
inline uint3 operator >> (uint3 x, u32 n)
{
    return { x.x >> n, x.y >> n, x.z >> n };
}
inline uint4 operator >> (uint4 x, u32 n)
{
    return { x.x >> n, x.y >> n, x.z >> n, x.w >> n };
}
inline ulong2 operator >> (ulong2 x, u32 n)
{
    return { x.x >> n, x.y >> n };
}
inline ulong3 operator >> (ulong3 x, u32 n)
{
    return { x.x >> n, x.y >> n, x.z >> n };
}
inline ulong4 operator >> (ulong4 x, u32 n)
{
    return { x.x >> n, x.y >> n, x.z >> n, x.w >> n };
}

inline uint2 operator | (uint2 a, uint2 b)
{
    return { a.x | b.x, a.y | b.y };
}
inline uint3 operator | (uint3 a, uint3 b)
{
    return { a.x | b.x, a.y | b.y, a.z | b.z };
}
inline uint4 operator | (uint4 a, uint4 b)
{
    return { a.x | b.x, a.y | b.y, a.z | b.z, a.w | b.w };
}
inline ulong2 operator | (ulong2 a, ulong2 b)
{
    return { a.x | b.x, a.y | b.y };
}
inline ulong3 operator | (ulong3 a, ulong3 b)
{
    return { a.x | b.x, a.y | b.y, a.z | b.z };
}
inline ulong4 operator | (ulong4 a, ulong4 b)
{
    return { a.x | b.x, a.y | b.y, a.z | b.z, a.w | b.w };
}

inline uint2 operator & (uint2 a, uint2 b)
{
    return { a.x & b.x, a.y & b.y };
}
inline uint3 operator & (uint3 a, uint3 b)
{
    return { a.x & b.x, a.y & b.y, a.z & b.z };
}
inline uint4 operator & (uint4 a, uint4 b)
{
    return { a.x & b.x, a.y & b.y, a.z & b.z, a.w & b.w };
}
inline ulong2 operator & (ulong2 a, ulong2 b)
{
    return { a.x & b.x, a.y & b.y };
}
inline ulong3 operator & (ulong3 a, ulong3 b)
{
    return { a.x & b.x, a.y & b.y, a.z & b.z };
}
inline ulong4 operator & (ulong4 a, ulong4 b)
{
    return { a.x & b.x, a.y & b.y, a.z & b.z, a.w & b.w };
}

inline uint2 operator ^ (uint2 a, uint2 b)
{
    return { a.x ^ b.x, a.y ^ b.y };
}
inline uint3 operator ^ (uint3 a, uint3 b)
{
    return { a.x ^ b.x, a.y ^ b.y, a.z ^ b.z };
}
inline uint4 operator ^ (uint4 a, uint4 b)
{
    return { a.x ^ b.x, a.y ^ b.y, a.z ^ b.z, a.w ^ b.w };
}
inline ulong2 operator ^ (ulong2 a, ulong2 b)
{
    return { a.x ^ b.x, a.y ^ b.y };
}
inline ulong3 operator ^ (ulong3 a, ulong3 b)
{
    return { a.x ^ b.x, a.y ^ b.y, a.z ^ b.z };
}
inline ulong4 operator ^ (ulong4 a, ulong4 b)
{
    return { a.x ^ b.x, a.y ^ b.y, a.z ^ b.z, a.w ^ b.w };
}

