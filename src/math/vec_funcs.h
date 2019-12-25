#pragma once

#ifndef NOMINMAX
    #define NOMINMAX
#endif

#include "math/vec_ops.h"
#include "common/apply.h"
#include <math.h>

namespace math
{
    constexpr f32 Pi = 3.141592f;
    constexpr f32 Tau = Pi * 2.0f;
    constexpr f32 HalfPi = Pi * 0.5f;

    inline f32 radians(f32 x) { return x * 0.017453293f; }
    inline float2 radians(float2 x) { return x * 0.017453293f; }
    inline float3 radians(float3 x) { return x * 0.017453293f; }
    inline float4 radians(float4 x) { return x * 0.017453293f; }

    inline f32 degrees(f32 x) { return x * 57.295779513f; }
    inline float2 degrees(float2 x) { return x * 57.295779513f; }
    inline float3 degrees(float3 x) { return x * 57.295779513f; }
    inline float4 degrees(float4 x) { return x * 57.295779513f; }

    inline f32 tan(f32 x)
    {
        return tanf(x);
    }
    inline float2 tan(float2 x)
    {
        return { tanf(x.x), tanf(x.y) };
    }
    inline float3 tan(float3 x)
    {
        return { tanf(x.x), tanf(x.y), tanf(x.z) };
    }
    inline float4 tan(float4 x)
    {
        return { tanf(x.x), tanf(x.y), tanf(x.z), tanf(x.w) };
    }

    inline f32 tanh(f32 x)
    {
        return tanhf(x);
    }
    inline float2 tanh(float2 x)
    {
        return { tanhf(x.x), tanhf(x.y) };
    }
    inline float3 tanh(float3 x)
    {
        return { tanhf(x.x), tanhf(x.y), tanhf(x.z) };
    }
    inline float4 tanh(float4 x)
    {
        return { tanhf(x.x), tanhf(x.y), tanhf(x.z), tanhf(x.w) };
    }

    inline f32 atan(f32 x)
    {
        return atanf(x);
    }
    inline float2 atan(float2 x)
    {
        return { atanf(x.x), atanf(x.y) };
    }
    inline float3 atan(float3 x)
    {
        return { atanf(x.x), atanf(x.y), atanf(x.z) };
    }
    inline float4 atan(float4 x)
    {
        return { atanf(x.x), atanf(x.y), atanf(x.z), atanf(x.w) };
    }

    inline f32 atan2(f32 x, f32 y)
    {
        return atan2f(x, y);
    }
    inline float2 atan2(float2 x, float2 y)
    {
        return { atan2f(x.x, y.x), atan2f(x.y, y.y) };
    }
    inline float3 atan2(float3 x, float3 y)
    {
        return
        {
            atan2f(x.x, y.x),
            atan2f(x.y, y.y),
            atan2f(x.z, y.z),
        };
    }
    inline float4 atan2(float4 x, float4 y)
    {
        return
        {
            atan2f(x.x, y.x),
            atan2f(x.y, y.y),
            atan2f(x.z, y.z),
            atan2f(x.w, y.w),
        };
    }

    inline f32 cos(f32 x)
    {
        return cosf(x);
    }
    inline float2 cos(float2 x)
    {
        return { cosf(x.x), cosf(x.y) };
    }
    inline float3 cos(float3 x)
    {
        return { cosf(x.x), cosf(x.y), cosf(x.z) };
    }
    inline float4 cos(float4 x)
    {
        return { cosf(x.x), cosf(x.y), cosf(x.z), cosf(x.w) };
    }

    inline f32 cosh(f32 x)
    {
        return coshf(x);
    }
    inline float2 cosh(float2 x)
    {
        return { coshf(x.x), coshf(x.y) };
    }
    inline float3 cosh(float3 x)
    {
        return { coshf(x.x), coshf(x.y), coshf(x.z) };
    }
    inline float4 cosh(float4 x)
    {
        return { coshf(x.x), coshf(x.y), coshf(x.z), coshf(x.w) };
    }

    inline f32 acos(f32 x)
    {
        return acosf(x);
    }
    inline float2 acos(float2 x)
    {
        return { acosf(x.x), acosf(x.y) };
    }
    inline float3 acos(float3 x)
    {
        return { acosf(x.x), acosf(x.y), acosf(x.z) };
    }
    inline float4 acos(float4 x)
    {
        return { acosf(x.x), acosf(x.y), acosf(x.z), acosf(x.w) };
    }

    inline f32 sin(f32 x)
    {
        return sinf(x);
    }
    inline float2 sin(float2 x)
    {
        return { sinf(x.x), sinf(x.y) };
    }
    inline float3 sin(float3 x)
    {
        return { sinf(x.x), sinf(x.y), sinf(x.z) };
    }
    inline float4 sin(float4 x)
    {
        return { sinf(x.x), sinf(x.y), sinf(x.z), sinf(x.w) };
    }

    inline f32 sinh(f32 x)
    {
        return sinhf(x);
    }
    inline float2 sinh(float2 x)
    {
        return { sinhf(x.x), sinhf(x.y) };
    }
    inline float3 sinh(float3 x)
    {
        return { sinhf(x.x), sinhf(x.y), sinhf(x.z) };
    }
    inline float4 sinh(float4 x)
    {
        return { sinhf(x.x), sinhf(x.y), sinhf(x.z), sinhf(x.w) };
    }

    inline f32 asin(f32 x)
    {
        return asinf(x);
    }
    inline float2 asin(float2 x)
    {
        return { asinf(x.x), asinf(x.y) };
    }
    inline float3 asin(float3 x)
    {
        return { asinf(x.x), asinf(x.y), asinf(x.z) };
    }
    inline float4 asin(float4 x)
    {
        return { asinf(x.x), asinf(x.y), asinf(x.z), asinf(x.w) };
    }

    inline void sincos(f32 x, f32& s, f32& c)
    {
        s = sin(x);
        c = cos(x);
    }
    inline void sincos(float2 x, float2& s, float2& c)
    {
        s = sin(x);
        c = cos(x);
    }
    inline void sincos(float3 x, float3& s, float3& c)
    {
        s = sin(x);
        c = cos(x);
    }
    inline void sincos(float4 x, float4& s, float4& c)
    {
        s = sin(x);
        c = cos(x);
    }

    // ------------------------------------------------------------------------

    inline f32 pow(f32 x, f32 y)
    {
        return powf(x, y);
    }
    inline float2 pow(float2 x, float2 y)
    {
        return
        {
            powf(x.x, y.x),
            powf(x.y, y.y),
        };
    }
    inline float3 pow(float3 x, float3 y)
    {
        return
        {
            powf(x.x, y.x),
            powf(x.y, y.y),
            powf(x.z, y.z),
        };
    }
    inline float4 pow(float4 x, float4 y)
    {
        return
        {
            powf(x.x, y.x),
            powf(x.y, y.y),
            powf(x.z, y.z),
            powf(x.w, y.w),
        };
    }

    inline f32 exp(f32 x) { return expf(x); }
    inline float2 exp(float2 x) { return { expf(x.x), expf(x.y) }; }
    inline float3 exp(float3 x) { return { expf(x.x), expf(x.y), expf(x.z) }; }
    inline float4 exp(float4 x) { return { expf(x.x), expf(x.y), expf(x.z), expf(x.w) }; }

    inline f32 exp2(f32 x) { return exp2f(x); }
    inline float2 exp2(float2 x) { return { exp2f(x.x), exp2f(x.y) }; }
    inline float3 exp2(float3 x) { return { exp2f(x.x), exp2f(x.y), exp2f(x.z) }; }
    inline float4 exp2(float4 x) { return { exp2f(x.x), exp2f(x.y), exp2f(x.z), exp2f(x.w) }; }

    inline f32 exp10(f32 x) { return exp(x * 2.3025851f); }
    inline float2 exp10(float2 x) { return exp(x * 2.3025851f); }
    inline float3 exp10(float3 x) { return exp(x * 2.3025851f); }
    inline float4 exp10(float4 x) { return exp(x * 2.3025851f); }

    inline f32 log(f32 x) { return logf(x); }
    inline float2 log(float2 x) { return { logf(x.x), logf(x.y) }; }
    inline float3 log(float3 x) { return { logf(x.x), logf(x.y), logf(x.z) }; }
    inline float4 log(float4 x) { return { logf(x.x), logf(x.y), logf(x.z), logf(x.w) }; }

    inline f32 log2(f32 x) { return log2f(x); }
    inline float2 log2(float2 x) { return { log2f(x.x), log2f(x.y) }; }
    inline float3 log2(float3 x) { return { log2f(x.x), log2f(x.y), log2f(x.z) }; }
    inline float4 log2(float4 x) { return { log2f(x.x), log2f(x.y), log2f(x.z), log2f(x.w) }; }

    inline f32 log10(f32 x) { return log10f(x); }
    inline float2 log10(float2 x) { return { log10f(x.x), log10f(x.y) }; }
    inline float3 log10(float3 x) { return { log10f(x.x), log10f(x.y), log10f(x.z) }; }
    inline float4 log10(float4 x) { return { log10f(x.x), log10f(x.y), log10f(x.z), log10f(x.w) }; }

    inline f32 sqrt(f32 x) { return sqrtf(x); }
    inline float2 sqrt(float2 x) { return { sqrtf(x.x), sqrtf(x.y) }; }
    inline float3 sqrt(float3 x) { return { sqrtf(x.x), sqrtf(x.y), sqrtf(x.z) }; }
    inline float4 sqrt(float4 x) { return { sqrtf(x.x), sqrtf(x.y), sqrtf(x.z), sqrtf(x.w) }; }

    inline f32 rsqrt(f32 x) { return 1.0f / sqrt(x); }
    inline float2 rsqrt(float2 x) { return 1.0f / sqrt(x); }
    inline float3 rsqrt(float3 x) { return 1.0f / sqrt(x); }
    inline float4 rsqrt(float4 x) { return 1.0f / sqrt(x); }

    // ------------------------------------------------------------------------

    inline f32 floor(f32 x)
    {
        return floorf(x);
    }
    inline float2 floor(float2 x)
    {
        return { floorf(x.x), floorf(x.y) };
    }
    inline float3 floor(float3 x)
    {
        return { floorf(x.x), floorf(x.y), floorf(x.z) };
    }
    inline float4 floor(float4 x)
    {
        return { floorf(x.x), floorf(x.y), floorf(x.z), floorf(x.w) };
    }

    inline f32 ceil(f32 x)
    {
        return ceilf(x);
    }
    inline float2 ceil(float2 x)
    {
        return { ceilf(x.x), ceilf(x.y) };
    }
    inline float3 ceil(float3 x)
    {
        return { ceilf(x.x), ceilf(x.y), ceilf(x.z) };
    }
    inline float4 ceil(float4 x)
    {
        return { ceilf(x.x), ceilf(x.y), ceilf(x.z), ceilf(x.w) };
    }

    inline f32 round(f32 x)
    {
        return roundf(x);
    }
    inline float2 round(float2 x)
    {
        return { roundf(x.x), roundf(x.y) };
    }
    inline float3 round(float3 x)
    {
        return { roundf(x.x), roundf(x.y), roundf(x.z) };
    }
    inline float4 round(float4 x)
    {
        return { roundf(x.x), roundf(x.y), roundf(x.z), roundf(x.w) };
    }

    inline f32 trunc(f32 x)
    {
        return truncf(x);
    }
    inline float2 trunc(float2 x)
    {
        return { truncf(x.x), truncf(x.y) };
    }
    inline float3 trunc(float3 x)
    {
        return { truncf(x.x), truncf(x.y), truncf(x.z) };
    }
    inline float4 trunc(float4 x)
    {
        return { truncf(x.x), truncf(x.y), truncf(x.z), truncf(x.w) };
    }

    inline f32 frac(f32 x)
    {
        return x - floor(x);
    }
    inline float2 frac(float2 x)
    {
        return x - floor(x);
    }
    inline float3 frac(float3 x)
    {
        return x - floor(x);
    }
    inline float4 frac(float4 x)
    {
        return x - floor(x);
    }

    inline f32 fmod(f32 x, f32 y) { return fmodf(x, y); }
    inline float2 fmod(float2 x, float2 y)
    {
        return
        {
            fmodf(x.x, y.x),
            fmodf(x.y, y.y),
        };
    }
    inline float3 fmod(float3 x, float3 y)
    {
        return
        {
            fmodf(x.x, y.x),
            fmodf(x.y, y.y),
            fmodf(x.z, y.z),
        };
    }
    inline float4 fmod(float4 x, float4 y)
    {
        return
        {
            fmodf(x.x, y.x),
            fmodf(x.y, y.y),
            fmodf(x.z, y.z),
            fmodf(x.w, y.w),
        };
    }

    inline f32 rcp(f32 x)
    {
        return 1.0f / x;
    }
    inline float2 rcp(float2 x)
    {
        return 1.0f / x;
    }
    inline float3 rcp(float3 x)
    {
        return 1.0f / x;
    }
    inline float4 rcp(float4 x)
    {
        return 1.0f / x;
    }

    inline f32 sign(f32 x)
    {
        return (x > 0.0f ? 1.0f : 0.0f) - (x < 0.0f ? 1.0f : 0.0f);
    }
    inline float2 sign(float2 x)
    {
        return { sign(x.x), sign(x.y) };
    }
    inline float3 sign(float3 x)
    {
        return { sign(x.x), sign(x.y), sign(x.z) };
    }
    inline float4 sign(float4 x)
    {
        return { sign(x.x), sign(x.y), sign(x.z), sign(x.w) };
    }

    // ------------------------------------------------------------------------

    template<typename T>
    inline T min(T a, T b)
    {
        return a < b ? a : b;
    }
    template<typename T>
    inline vec2<T> min(vec2<T> a, vec2<T> b)
    {
        return { min(a.x, b.x), min(a.y, b.y) };
    }
    template<typename T>
    inline vec3<T> min(vec3<T> a, vec3<T> b)
    {
        return
        {
            min(a.x, b.x),
            min(a.y, b.y),
            min(a.z, b.z)
        };
    }
    template<typename T>
    inline vec4<T> min(vec4<T> a, vec4<T> b)
    {
        return
        {
            min(a.x, b.x),
            min(a.y, b.y),
            min(a.z, b.z),
            min(a.w, b.w)
        };
    }

    template<typename T>
    inline T max(T a, T b)
    {
        return a > b ? a : b;
    }
    template<typename T>
    inline vec2<T> max(vec2<T> a, vec2<T> b)
    {
        return { max(a.x, b.x), max(a.y, b.y) };
    }
    template<typename T>
    inline vec3<T> max(vec3<T> a, vec3<T> b)
    {
        return
        {
            max(a.x, b.x),
            max(a.y, b.y),
            max(a.z, b.z)
        };
    }
    template<typename T>
    inline vec4<T> max(vec4<T> a, vec4<T> b)
    {
        return
        {
            max(a.x, b.x),
            max(a.y, b.y),
            max(a.z, b.z),
            max(a.w, b.w)
        };
    }

    template<typename T>
    inline T cmin(vec2<T> x)
    {
        return min(x.x, x.y);
    }
    template<typename T>
    inline T cmin(vec3<T> x)
    {
        return min(x.x, min(x.y, x.z));
    }
    template<typename T>
    inline T cmin(vec4<T> x)
    {
        return min(x.x, min(x.y, min(x.z, x.w)));
    }

    template<typename T>
    inline T cmax(vec2<T> x)
    {
        return max(x.x, x.y);
    }
    template<typename T>
    inline T cmax(vec3<T> x)
    {
        return max(x.x, max(x.y, x.z));
    }
    template<typename T>
    inline T cmax(vec4<T> x)
    {
        return max(x.x, max(x.y, max(x.z, x.w)));
    }

    template<typename T>
    inline T csum(vec2<T> x)
    {
        return x.x + x.y;
    }
    template<typename T>
    inline T csum(vec3<T> x)
    {
        return x.x + x.y + x.z;
    }
    template<typename T>
    inline T csum(vec4<T> x)
    {
        return x.x + x.y + x.z + x.w;
    }

    template<typename T>
    inline T cmul(vec2<T> x)
    {
        return x.x * x.y;
    }
    template<typename T>
    inline T cmul(vec3<T> x)
    {
        return x.x * x.y * x.z;
    }
    template<typename T>
    inline T cmul(vec4<T> x)
    {
        return x.x * x.y * x.z * x.w;
    }

    template<typename T>
    inline T clamp(T x, T lo, T hi)
    {
        return min(hi, max(lo, x));
    }

    inline f32 saturate(f32 x)
    {
        return clamp(x, 0.0f, 1.0f);
    }
    inline float2 saturate(float2 x)
    {
        return { saturate(x.x), saturate(x.y) };
    }
    inline float3 saturate(float3 x)
    {
        return { saturate(x.x), saturate(x.y), saturate(x.z) };
    }
    inline float4 saturate(float4 x)
    {
        return { saturate(x.x), saturate(x.y), saturate(x.z), saturate(x.w) };
    }

    template<typename T>
    inline T abs(T x)
    {
        return max(x, -x);
    }

    template<typename T>
    inline T reinhard(T x)
    {
        return x / (1.0f + abs(x));
    }

    template<typename T>
    inline T to_unorm(T x)
    {
        return 0.5f + 0.5f * x;
    }

    template<typename T>
    inline T to_snorm(T x)
    {
        return -1.0f + 2.0f * x;
    }

    // ----------------------------------------------------------------------------

    // [F, T] -> [a, b]
    template<typename T>
    inline T select(T a, T b, bool x)
    {
        return x ? b : a;
    }
    template<typename T>
    inline vec2<T> select(vec2<T> a, vec2<T> b, bool2 x)
    {
        return
        {
            select(a.x, b.x, x.x),
            select(a.y, b.y, x.y),
        };
    }
    template<typename T>
    inline vec3<T> select(vec3<T> a, vec3<T> b, bool3 x)
    {
        return
        {
            select(a.x, b.x, x.x),
            select(a.y, b.y, x.y),
            select(a.z, b.z, x.z),
        };
    }
    template<typename T>
    inline vec4<T> select(vec4<T> a, vec4<T> b, bool4 x)
    {
        return
        {
            select(a.x, b.x, x.x),
            select(a.y, b.y, x.y),
            select(a.z, b.z, x.z),
            select(a.w, b.w, x.w),
        };
    }

    template<typename T>
    inline T step(T x, T y)
    {
        return select(T(0.0f), T(1.0f), x >= y);
    }

    template<typename T>
    inline T smoothstep(T a, T b, T x)
    {
        T t = saturate((x - a) / (b - a));
        return t * t * (3.0f - (2.0f * t));
    }

    // [0, 1] -> [a, b]
    inline f32 lerp(f32 a, f32 b, f32 x)
    {
        return a + x * (b - a);
    }
    inline float2 lerp(float2 a, float2 b, f32 x)
    {
        return a + x * (b - a);
    }
    inline float2 lerp(float2 a, float2 b, float2 x)
    {
        return a + x * (b - a);
    }
    inline float3 lerp(float3 a, float3 b, f32 x)
    {
        return a + x * (b - a);
    }
    inline float3 lerp(float3 a, float3 b, float3 x)
    {
        return a + x * (b - a);
    }
    inline float4 lerp(float4 a, float4 b, f32 x)
    {
        return a + x * (b - a);
    }
    inline float4 lerp(float4 a, float4 b, float4 x)
    {
        return a + x * (b - a);
    }

    // [a, b] -> [0, 1]
    inline f32 unlerp(f32 a, f32 b, f32 x)
    {
        return (x - a) / (b - a);
    }
    inline float2 unlerp(float2 a, float2 b, f32 x)
    {
        return (x - a) / (b - a);
    }
    inline float2 unlerp(float2 a, float2 b, float2 x)
    {
        return (x - a) / (b - a);
    }
    inline float3 unlerp(float3 a, float3 b, f32 x)
    {
        return (x - a) / (b - a);
    }
    inline float3 unlerp(float3 a, float3 b, float3 x)
    {
        return (x - a) / (b - a);
    }
    inline float4 unlerp(float4 a, float4 b, f32 x)
    {
        return (x - a) / (b - a);
    }
    inline float4 unlerp(float4 a, float4 b, float4 x)
    {
        return (x - a) / (b - a);
    }

    // ------------------------------------------------------------------------

    template<typename T>
    inline T dot(T x, T y)
    {
        return x * y;
    }
    template<typename T>
    inline T dot(vec2<T> x, vec2<T> y)
    {
        return x.x * y.x + x.y * y.y;
    }
    template<typename T>
    inline T dot(vec3<T> x, vec3<T> y)
    {
        return x.x * y.x + x.y * y.y + x.z * y.z;
    }
    template<typename T>
    inline T dot(vec4<T> x, vec4<T> y)
    {
        return x.x * y.x + x.y * y.y + x.z * y.z + x.w * y.w;
    }

    inline float3 cross(float3 x, float3 y)
    {
        float3 a = x * float3(y.y, y.z, y.x) - float3(x.y, x.z, x.x) * y;
        return float3(a.y, a.z, a.x);
    }

    inline f32 reflect(f32 i, f32 n) { return i - 2.0f * n * dot(i, n); }
    inline float2 reflect(float2 i, float2 n) { return i - 2.0f * n * dot(i, n); }
    inline float3 reflect(float3 i, float3 n) { return i - 2.0f * n * dot(i, n); }
    inline float4 reflect(float4 i, float4 n) { return i - 2.0f * n * dot(i, n); }

    inline f32 refract(f32 i, f32 n, f32 ior)
    {
        f32 noi = dot(n, i);
        f32 k = 1.0f - ior * ior * (1.0f - noi * noi);
        f32 b = ior * i - (ior * noi + sqrt(k)) * n;
        return select(0.0f, b, k >= 0.0f);
    }
    inline float2 refract(float2 i, float2 n, f32 ior)
    {
        f32 noi = dot(n, i);
        f32 k = 1.0f - ior * ior * (1.0f - noi * noi);
        float2 a = 0.0f;
        float2 b = ior * i - (ior * noi + sqrt(k)) * n;
        return select(a, b, k >= 0.0f);
    }
    inline float3 refract(float3 i, float3 n, f32 ior)
    {
        f32 noi = dot(n, i);
        f32 k = 1.0f - ior * ior * (1.0f - noi * noi);
        float3 a = 0.0f;
        float3 b = ior * i - (ior * noi + sqrt(k)) * n;
        return select(a, b, k >= 0.0f);
    }
    inline float4 refract(float4 i, float4 n, f32 ior)
    {
        f32 noi = dot(n, i);
        f32 k = 1.0f - ior * ior * (1.0f - noi * noi);
        float4 a = 0.0f;
        float4 b = ior * i - (ior * noi + sqrt(k)) * n;
        return select(a, b, k >= 0.0f);
    }

    // ----------------------------------------------------------------------------

    inline float2 normalize(float2 x) { return rsqrt(dot(x, x)) * x; }
    inline float3 normalize(float3 x) { return rsqrt(dot(x, x)) * x; }
    inline float4 normalize(float4 x) { return rsqrt(dot(x, x)) * x; }

    inline f32 length(f32 x) { return abs(x); }
    inline f32 length(float2 x) { return sqrt(dot(x, x)); }
    inline f32 length(float3 x) { return sqrt(dot(x, x)); }
    inline f32 length(float4 x) { return sqrt(dot(x, x)); }

    inline f32 lengthsq(f32 x) { return x * x; }
    inline f32 lengthsq(float2 x) { return dot(x, x); }
    inline f32 lengthsq(float3 x) { return dot(x, x); }
    inline f32 lengthsq(float4 x) { return dot(x, x); }

    inline f32 distance(f32 x, f32 y) { return length(y - x); }
    inline f32 distance(float2 x, float2 y) { return length(y - x); }
    inline f32 distance(float3 x, float3 y) { return length(y - x); }
    inline f32 distance(float4 x, float4 y) { return length(y - x); }

    inline f32 distancesq(f32 x, f32 y) { return lengthsq(y - x); }
    inline f32 distancesq(float2 x, float2 y) { return lengthsq(y - x); }
    inline f32 distancesq(float3 x, float3 y) { return lengthsq(y - x); }
    inline f32 distancesq(float4 x, float4 y) { return lengthsq(y - x); }

    // ----------------------------------------------------------------------------

    inline u32 rol(u32 x, u32 n) { return (x << n) | (x >> (32u - n)); }
    inline uint2 rol(uint2 x, u32 n) { return (x << n) | (x >> (32u - n)); }
    inline uint3 rol(uint3 x, u32 n) { return (x << n) | (x >> (32u - n)); }
    inline uint4 rol(uint4 x, u32 n) { return (x << n) | (x >> (32u - n)); }

    inline u64 rol(u64 x, u32 n) { return (x << n) | (x >> (64u - n)); }
    inline ulong2 rol(ulong2 x, u32 n) { return (x << n) | (x >> (64u - n)); }
    inline ulong3 rol(ulong3 x, u32 n) { return (x << n) | (x >> (64u - n)); }
    inline ulong4 rol(ulong4 x, u32 n) { return (x << n) | (x >> (64u - n)); }

    inline u32 ror(u32 x, u32 n) { return (x >> n) | (x << (32u - n)); }
    inline uint2 ror(uint2 x, u32 n) { return (x >> n) | (x << (32u - n)); }
    inline uint3 ror(uint3 x, u32 n) { return (x >> n) | (x << (32u - n)); }
    inline uint4 ror(uint4 x, u32 n) { return (x >> n) | (x << (32u - n)); }

    inline u64 ror(u64 x, u32 n) { return (x >> n) | (x << (64u - n)); }
    inline ulong2 ror(ulong2 x, u32 n) { return (x >> n) | (x << (64u - n)); }
    inline ulong3 ror(ulong3 x, u32 n) { return (x >> n) | (x << (64u - n)); }
    inline ulong4 ror(ulong4 x, u32 n) { return (x >> n) | (x << (64u - n)); }

#pragma warning(push)
#pragma warning(disable : 4307)
#pragma warning(disable : 4146)
    inline u32 hash(void* pBuffer, u32 numBytes, u32 seed = 0)
    {
        ASSERT(pBuffer || !numBytes);

        constexpr u32 Prime1 = 2654435761u;
        constexpr u32 Prime2 = 2246822519u;
        constexpr u32 Prime3 = 3266489917u;
        constexpr u32 Prime4 = 668265263u;
        constexpr u32 Prime5 = 374761393u;

        const u32 count16 = numBytes >> 4;
        const u32 count4 = (numBytes >> 2) & 3;
        const u32 count1 = numBytes & 3;

        uint4* p16 = (uint4*)pBuffer;
        u32 x = seed + Prime5;

        if (count16 > 0)
        {
            uint4 state = uint4(Prime1 + Prime2, Prime2, 0, -Prime1) + seed;
            for (u32 i = 0; i < count16; ++i)
            {
                state += *p16++ * Prime2;
                state = (state << 13) | (state >> 19);
                state *= Prime1;
            }
            x = rol(state.x, 1) + rol(state.y, 7) + rol(state.z, 12) + rol(state.w, 18);
        }

        x += numBytes;

        u32* p4 = (u32*)p16;
        for (u32 i = 0; i < count4; ++i)
        {
            x += *p4++ * Prime3;
            x = rol(x, 17) * Prime4;
        }

        u8* p1 = (u8*)p4;
        for (u32 i = 0; i < count1; ++i)
        {
            x += *p1++ * Prime5;
            x = rol(x, 11) * Prime1;
        }

        x ^= x >> 15;
        x *= Prime2;
        x ^= x >> 13;
        x *= Prime3;
        x ^= x >> 16;

        return x;
    }
#pragma warning(pop)

    // ----------------------------------------------------------------------------

    template<typename T>
    inline vec2<T> swizzle(vec2<T> v, i32 a, i32 b)
    {
        return { v[a], v[b] };
    }
    template<typename T>
    inline vec3<T> swizzle(vec2<T> v, i32 a, i32 b, i32 c)
    {
        return { v[a], v[b], v[c] };
    }
    template<typename T>
    inline vec4<T> swizzle(vec2<T> v, i32 a, i32 b, i32 c, i32 d)
    {
        return { v[a], v[b], v[c], v[d] };
    }

    template<typename T>
    inline vec2<T> swizzle(vec3<T> v, i32 a, i32 b)
    {
        return { v[a], v[b] };
    }
    template<typename T>
    inline vec3<T> swizzle(vec3<T> v, i32 a, i32 b, i32 c)
    {
        return { v[a], v[b], v[c] };
    }
    template<typename T>
    inline vec4<T> swizzle(vec3<T> v, i32 a, i32 b, i32 c, i32 d)
    {
        return { v[a], v[b], v[c], v[d] };
    }

    template<typename T>
    inline vec2<T> swizzle(vec4<T> v, i32 a, i32 b)
    {
        return { v[a], v[b] };
    }
    template<typename T>
    inline vec3<T> swizzle(vec4<T> v, i32 a, i32 b, i32 c)
    {
        return { v[a], v[b], v[c] };
    }
    template<typename T>
    inline vec4<T> swizzle(vec4<T> v, i32 a, i32 b, i32 c, i32 d)
    {
        return { v[a], v[b], v[c], v[d] };
    }

    // ----------------------------------------------------------------------------

    inline u32 any(bool2 x)
    {
        return x.x | x.y;
    }
    inline u32 any(bool3 x)
    {
        return x.x | x.y | x.z;
    }
    inline u32 any(bool4 x)
    {
        return x.x | x.y | x.z | x.w;
    }

    inline u32 all(bool2 x)
    {
        return x.x & x.y;
    }
    inline u32 all(bool3 x)
    {
        return x.x & x.y & x.z;
    }
    inline u32 all(bool4 x)
    {
        return x.x & x.y & x.z & x.w;
    }

    inline u32 none(bool2 x)
    {
        return ~any(x);
    }
    inline u32 none(bool3 x)
    {
        return ~any(x);
    }
    inline u32 none(bool4 x)
    {
        return ~any(x);
    }

    // ----------------------------------------------------------------------------

}; // math
