#pragma once

#include "math/vec_types.h"

// ----------------------------------------------------------------------------

template<typename T>
inline vec2<T> operator - (vec2<T> x)
{
    return { -x.x, -x.y };
}
template<typename T>
inline vec3<T> operator - (vec3<T> x)
{
    return { -x.x, -x.y, -x.z };
}
template<typename T>
inline vec4<T> operator - (vec4<T> x)
{
    return { -x.x, -x.y, -x.z, -x.w };
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

// --------------------------------------------------------

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

// --------------------------------------------------------

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

// --------------------------------------------------------

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

// --------------------------------------------------------

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

// --------------------------------------------------------

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

// --------------------------------------------------------

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

// --------------------------------------------------------

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

// ----------------------------------------------------------------------------

template<typename T>
inline vec2<T> operator ~ (vec2<T> a)
{
    return { ~a.x, ~a.y };
}
template<typename T>
inline vec3<T> operator ~ (vec3<T> a)
{
    return { ~a.x, ~a.y, ~a.z };
}
template<typename T>
inline vec4<T> operator ~ (vec4<T> a)
{
    return { ~a.x, ~a.y, ~a.z, ~a.w };
}

// --------------------------------------------------------

template<typename T>
inline vec2<T> operator << (vec2<T> x, i32 n)
{
    return { x.x << n, x.y << n };
}
template<typename T>
inline vec3<T> operator << (vec3<T> x, i32 n)
{
    return { x.x << n, x.y << n, x.z << n };
}
template<typename T>
inline vec4<T> operator << (vec4<T> x, i32 n)
{
    return { x.x << n, x.y << n, x.z << n, x.w << n };
}

// --------------------------------------------------------

template<typename T>
inline vec2<T> operator >> (vec2<T> x, i32 n)
{
    return { x.x >> n, x.y >> n };
}
template<typename T>
inline vec3<T> operator >> (vec3<T> x, i32 n)
{
    return { x.x >> n, x.y >> n, x.z >> n };
}
template<typename T>
inline vec4<T> operator >> (vec4<T> x, i32 n)
{
    return { x.x >> n, x.y >> n, x.z >> n, x.w >> n };
}

// --------------------------------------------------------

template<typename T>
inline vec2<T> operator | (vec2<T> a, vec2<T> b)
{
    return { a.x | b.x, a.y | b.y };
}
template<typename T>
inline vec3<T> operator | (vec3<T> a, vec3<T> b)
{
    return { a.x | b.x, a.y | b.y, a.z | b.z };
}
template<typename T>
inline vec4<T> operator | (vec4<T> a, vec4<T> b)
{
    return { a.x | b.x, a.y | b.y, a.z | b.z, a.w | b.w };
}

template<typename T>
inline vec2<T> operator | (vec2<T> a, T b)
{
    return { a.x | b, a.y | b };
}
template<typename T>
inline vec3<T> operator | (vec3<T> a, T b)
{
    return { a.x | b, a.y | b, a.z | b };
}
template<typename T>
inline vec4<T> operator | (vec4<T> a, T b)
{
    return { a.x | b, a.y | b, a.z | b, a.w | b };
}

template<typename T>
inline vec2<T> operator | (T a, vec2<T> b)
{
    return { a | b.x, a | b.y };
}
template<typename T>
inline vec3<T> operator | (T a, vec3<T> b)
{
    return { a | b.x, a | b.y, a | b.z };
}
template<typename T>
inline vec4<T> operator | (T a, vec4<T> b)
{
    return { a | b.x, a | b.y, a | b.z, a | b.w };
}

// --------------------------------------------------------

template<typename T>
inline vec2<T> operator & (vec2<T> a, vec2<T> b)
{
    return { a.x & b.x, a.y & b.y };
}
template<typename T>
inline vec3<T> operator & (vec3<T> a, vec3<T> b)
{
    return { a.x & b.x, a.y & b.y, a.z & b.z };
}
template<typename T>
inline vec4<T> operator & (vec4<T> a, vec4<T> b)
{
    return { a.x & b.x, a.y & b.y, a.z & b.z, a.w & b.w };
}

template<typename T>
inline vec2<T> operator & (vec2<T> a, T b)
{
    return { a.x & b, a.y & b };
}
template<typename T>
inline vec3<T> operator & (vec3<T> a, T b)
{
    return { a.x & b, a.y & b, a.z & b };
}
template<typename T>
inline vec4<T> operator & (vec4<T> a, T b)
{
    return { a.x & b, a.y & b, a.z & b, a.w & b };
}

template<typename T>
inline vec2<T> operator & (T a, vec2<T> b)
{
    return { a & b.x, a & b.y };
}
template<typename T>
inline vec3<T> operator & (T a, vec3<T> b)
{
    return { a & b.x, a & b.y, a & b.z };
}
template<typename T>
inline vec4<T> operator & (T a, vec4<T> b)
{
    return { a & b.x, a & b.y, a & b.z, a & b.w };
}

// --------------------------------------------------------

template<typename T>
inline vec2<T> operator ^ (vec2<T> a, vec2<T> b)
{
    return { a.x ^ b.x, a.y ^ b.y };
}
template<typename T>
inline vec3<T> operator ^ (vec3<T> a, vec3<T> b)
{
    return { a.x ^ b.x, a.y ^ b.y, a.z ^ b.z };
}
template<typename T>
inline vec4<T> operator ^ (vec4<T> a, vec4<T> b)
{
    return { a.x ^ b.x, a.y ^ b.y, a.z ^ b.z, a.w ^ b.w };
}

template<typename T>
inline vec2<T> operator ^ (vec2<T> a, T b)
{
    return { a.x ^ b, a.y ^ b };
}
template<typename T>
inline vec3<T> operator ^ (vec3<T> a, T b)
{
    return { a.x ^ b, a.y ^ b, a.z ^ b };
}
template<typename T>
inline vec4<T> operator ^ (vec4<T> a, T b)
{
    return { a.x ^ b, a.y ^ b, a.z ^ b, a.w ^ b };
}

template<typename T>
inline vec2<T> operator ^ (T a, vec2<T> b)
{
    return { a ^ b.x, a ^ b.y };
}
template<typename T>
inline vec3<T> operator ^ (T a, vec3<T> b)
{
    return { a ^ b.x, a ^ b.y, a ^ b.z };
}
template<typename T>
inline vec4<T> operator ^ (T a, vec4<T> b)
{
    return { a ^ b.x, a ^ b.y, a ^ b.z, a ^ b.w };
}

// --------------------------------------------------------
