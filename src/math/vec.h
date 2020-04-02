#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#define kPi                 3.141592653f
#define kTau                6.283185307f
#define kRadiansPerDegree   (kTau / 360.0f)
#define kDegreesPerRadian   (360.0f / kTau)

#if PLAT_CPU_X86

#include <xmmintrin.h>
#include <emmintrin.h>

typedef struct vec_s
{
    __m128 m;
} vec_t;

typedef union vec_bits_s
{
    uint32_t u[4];
    __m128 v;
} vec_bits_t;

static const vec_bits_t kVecSignBits =
{ 0x80000000u, 0x80000000u, 0x80000000u, 0x80000000u };

static vec_t VEC_CALL vec_set(float x, float y, float z, float w)
{
    vec_t vec = { _mm_set_ps(x, y, z, w) };
    return vec;
}

static vec_t VEC_CALL vec_set1(float s)
{
    vec_t vec = { _mm_set_ps1(s) };
    return vec;
}

static vec_t VEC_CALL vec_zero(void)
{
    vec_t vec = { _mm_setzero_ps() };
    return vec;
}

static vec_t VEC_CALL vec_load(const float* src)
{
    vec_t dst = { _mm_loadu_ps(src) };
    return dst;
}

static void VEC_CALL vec_store(vec_t src, float* dst)
{
    _mm_store_ps(dst, src.m);
}

#define vec_shuf(v, x, y, z, w)     _mm_shuffle_ps(v.m, v.m, _MM_SHUFFLE(w, z, y, x))

static float VEC_CALL vec_x(vec_t v)
{
    return _mm_cvtss_f32(v.m);
}

static float VEC_CALL vec_y(vec_t v)
{
    return _mm_cvtss_f32(vec_shuf(v, 1, 1, 1, 1));
}

static float VEC_CALL vec_z(vec_t v)
{
    return _mm_cvtss_f32(vec_shuf(v, 2, 2, 2, 2));
}

static float VEC_CALL vec_w(vec_t v)
{
    return _mm_cvtss_f32(vec_shuf(v, 3, 3, 3, 3));
}

static vec_t VEC_CALL vec_zxy(vec_t x)
{
    x.m = vec_shuf(x, 2, 0, 1, 0);
    return x;
}

static vec_t VEC_CALL vec_yzx(vec_t x)
{
    x.m = vec_shuf(x, 1, 2, 0, 0);
    return x;
}

static vec_t VEC_CALL vec_add(vec_t lhs, vec_t rhs)
{
    lhs.m = _mm_add_ps(lhs.m, rhs.m);
    return lhs;
}

static vec_t VEC_CALL vec_sub(vec_t lhs, vec_t rhs)
{
    lhs.m = _mm_sub_ps(lhs.m, rhs.m);
    return lhs;
}

static vec_t VEC_CALL vec_mul(vec_t lhs, vec_t rhs)
{
    lhs.m = _mm_mul_ps(lhs.m, rhs.m);
    return lhs;
}

static vec_t VEC_CALL vec_div(vec_t lhs, vec_t rhs)
{
    lhs.m = _mm_div_ps(lhs.m, rhs.m);
    return lhs;
}

static vec_t VEC_CALL vec_eq(vec_t lhs, vec_t rhs)
{
    vec_t y = { _mm_cmpeq_ps(lhs.m, rhs.m) };
    return y;
}

static vec_t VEC_CALL vec_neq(vec_t lhs, vec_t rhs)
{
    vec_t y = { _mm_cmpneq_ps(lhs.m, rhs.m) };
    return y;
}

static vec_t VEC_CALL vec_lt(vec_t lhs, vec_t rhs)
{
    vec_t y = { _mm_cmplt_ps(lhs.m, rhs.m) };
    return y;
}

static vec_t VEC_CALL vec_gt(vec_t lhs, vec_t rhs)
{
    vec_t y = { _mm_cmpgt_ps(lhs.m, rhs.m) };
    return y;
}

static vec_t VEC_CALL vec_lteq(vec_t lhs, vec_t rhs)
{
    vec_t y = { _mm_cmple_ps(lhs.m, rhs.m) };
    return y;
}

static vec_t VEC_CALL vec_gteq(vec_t lhs, vec_t rhs)
{
    vec_t y = { _mm_cmpge_ps(lhs.m, rhs.m) };
    return y;
}

static vec_t VEC_CALL vec_and(vec_t lhs, vec_t rhs)
{
    vec_t y = { _mm_and_ps(lhs.m, rhs.m) };
    return y;
}

static vec_t VEC_CALL vec_or(vec_t lhs, vec_t rhs)
{
    vec_t y = { _mm_or_ps(lhs.m, rhs.m) };
    return y;
}

static vec_t VEC_CALL vec_xor(vec_t lhs, vec_t rhs)
{
    vec_t y = { _mm_xor_ps(lhs.m, rhs.m) };
    return y;
}

static vec_t VEC_CALL vec_neg(vec_t x)
{
    x.m = _mm_sub_ps(_mm_setzero_ps(), x.m);
    return x;
}

static vec_t VEC_CALL vec_rcp(vec_t x)
{
    x.m = _mm_rcp_ps(x.m);
    return x;
}

static vec_t VEC_CALL vec_sqrt(vec_t x)
{
    x.m = _mm_sqrt_ps(x.m);
    return x;
}

static vec_t VEC_CALL vec_abs(vec_t x)
{
    x.m = _mm_andnot_ps(kVecSignBits.v, x.m);
    return x;
}

static vec_t VEC_CALL vec_min(vec_t lhs, vec_t rhs)
{
    lhs.m = _mm_min_ps(lhs.m, rhs.m);
    return lhs;
}

static vec_t VEC_CALL vec_max(vec_t lhs, vec_t rhs)
{
    lhs.m = _mm_max_ps(lhs.m, rhs.m);
    return lhs;
}

static uint32_t VEC_CALL vec_mask(vec_t x)
{
    return _mm_movemask_ps(x.m) & 15;
}

static bool VEC_CALL vec_any(vec_t x)
{
    return vec_mask(x) != 0;
}

static bool VEC_CALL vec_all(vec_t x)
{
    return vec_mask(x) == 15;
}

static vec_t VEC_CALL vec_select(vec_t a, vec_t b, vec_t t)
{
    __m128 d = _mm_castsi128_ps(_mm_srai_epi32(_mm_castps_si128(t.m), 31));
    a.m = _mm_or_ps(_mm_and_ps(d, b.m), _mm_andnot_ps(d, a.m));
    return a;
}

#else

typedef struct vec_s
{

} vec_t;

#endif // PLAT_CPU

// non-vectorized functions:

static vec_t VEC_CALL vec_pow(vec_t x, vec_t e)
{
    return vec_set(
        powf(vec_x(x), vec_x(e)),
        powf(vec_y(x), vec_y(e)),
        powf(vec_z(x), vec_z(e)),
        powf(vec_w(x), vec_w(e)));
}

static vec_t VEC_CALL vec_exp(vec_t x)
{
    return vec_set(
        expf(vec_x(x)),
        expf(vec_y(x)),
        expf(vec_z(x)),
        expf(vec_w(x)));
}

static vec_t VEC_CALL vec_log(vec_t x)
{
    return vec_set(
        logf(vec_x(x)),
        logf(vec_y(x)),
        logf(vec_z(x)),
        logf(vec_w(x)));
}

static vec_t VEC_CALL vec_sin(vec_t x)
{
    return vec_set(
        sinf(vec_x(x)),
        sinf(vec_y(x)),
        sinf(vec_z(x)),
        sinf(vec_w(x)));
}

static vec_t VEC_CALL vec_cos(vec_t x)
{
    return vec_set(
        cosf(vec_x(x)),
        cosf(vec_y(x)),
        cosf(vec_z(x)),
        cosf(vec_w(x)));
}

static vec_t VEC_CALL vec_tan(vec_t x)
{
    return vec_set(
        tanf(vec_x(x)),
        tanf(vec_y(x)),
        tanf(vec_z(x)),
        tanf(vec_w(x)));
}
static vec_t VEC_CALL vec_asin(vec_t x)
{
    return vec_set(
        asinf(vec_x(x)),
        asinf(vec_y(x)),
        asinf(vec_z(x)),
        asinf(vec_w(x)));
}

static vec_t VEC_CALL vec_acos(vec_t x)
{
    return vec_set(
        acosf(vec_x(x)),
        acosf(vec_y(x)),
        acosf(vec_z(x)),
        acosf(vec_w(x)));
}

static vec_t VEC_CALL vec_atan(vec_t x)
{
    return vec_set(
        atanf(vec_x(x)),
        atanf(vec_y(x)),
        atanf(vec_z(x)),
        atanf(vec_w(x)));
}

static vec_t VEC_CALL vec_floor(vec_t x)
{
    return vec_set(
        floorf(vec_x(x)),
        floorf(vec_y(x)),
        floorf(vec_z(x)),
        floorf(vec_w(x)));
}

static vec_t VEC_CALL vec_ceil(vec_t x)
{
    return vec_set(
        ceilf(vec_x(x)),
        ceilf(vec_y(x)),
        ceilf(vec_z(x)),
        ceilf(vec_w(x)));
}

static vec_t VEC_CALL vec_trunc(vec_t x)
{
    return vec_set(
        truncf(vec_x(x)),
        truncf(vec_y(x)),
        truncf(vec_z(x)),
        truncf(vec_w(x)));
}

static vec_t VEC_CALL vec_frac(vec_t v)
{
    float x = vec_x(v);
    float y = vec_y(v);
    float z = vec_z(v);
    float w = vec_w(v);
    x = x - floorf(x);
    y = y - floorf(y);
    z = z - floorf(z);
    w = w - floorf(w);
    return vec_set(x, y, z, w);
}

static vec_t VEC_CALL vec_fmod(vec_t a, vec_t b)
{
    return vec_set(
        fmodf(vec_x(a), vec_x(b)),
        fmodf(vec_y(a), vec_y(b)),
        fmodf(vec_z(a), vec_z(b)),
        fmodf(vec_w(a), vec_w(b)));
}

// composite functions:

static vec_t VEC_CALL vec_one(void)
{
    return vec_set1(1.0f);
}

static vec_t VEC_CALL vec_two(void)
{
    return vec_set1(2.0f);
}

static vec_t VEC_CALL vec_radians(vec_t x)
{
    return vec_mul(x, vec_set1(kRadiansPerDegree));
}

static vec_t VEC_CALL vec_degrees(vec_t x)
{
    return vec_mul(x, vec_set1(kDegreesPerRadian));
}

static float VEC_CALL vec_hmin(vec_t x)
{
    x = vec_min(x, { vec_shuf(x, 1, 0, 0, 0) });
    x = vec_min(x, { vec_shuf(x, 2, 0, 0, 0) });
    x = vec_min(x, { vec_shuf(x, 3, 0, 0, 0) });
    return vec_x(x);
}

static float VEC_CALL vec_hmax(vec_t x)
{
    x = vec_max(x, { vec_shuf(x, 1, 0, 0, 0) });
    x = vec_max(x, { vec_shuf(x, 2, 0, 0, 0) });
    x = vec_max(x, { vec_shuf(x, 3, 0, 0, 0) });
    return vec_x(x);
}

static vec_t VEC_CALL vec3_cross(vec_t a, vec_t b)
{
    return vec_zxy(
        vec_sub(
            vec_mul(vec_zxy(a), b),
            vec_mul(a, vec_zxy(b))));
}

static vec_t VEC_CALL vec_clamp(vec_t x, vec_t lo, vec_t hi)
{
    return vec_min(vec_max(x, lo), hi);
}

static float VEC_CALL vec4_sum(vec_t x)
{
    return vec_x(x) + vec_y(x) + vec_z(x) + vec_w(x);
}
static float VEC_CALL vec3_sum(vec_t x)
{
    return vec_x(x) + vec_y(x) + vec_z(x);
}
static float VEC_CALL vec2_sum(vec_t x)
{
    return vec_x(x) + vec_y(x);
}

static float VEC_CALL vec4_dot(vec_t a, vec_t b)
{
    return vec4_sum(vec_mul(a, b));
}
static float VEC_CALL vec3_dot(vec_t a, vec_t b)
{
    return vec3_sum(vec_mul(a, b));
}
static float VEC_CALL vec2_dot(vec_t a, vec_t b)
{
    return vec2_sum(vec_mul(a, b));
}

static float VEC_CALL vec4_length(vec_t x)
{
    return sqrtf(vec4_dot(x, x));
}
static float VEC_CALL vec3_length(vec_t x)
{
    return sqrtf(vec3_dot(x, x));
}
static float VEC_CALL vec2_length(vec_t x)
{
    return sqrtf(vec2_dot(x, x));
}

static vec_t VEC_CALL vec4_normalize(vec_t x)
{
    return vec_mul(x, vec_set1(1.0f / vec4_length(x)));
}
static vec_t VEC_CALL vec3_normalize(vec_t x)
{
    return vec_mul(x, vec_set1(1.0f / vec3_length(x)));
}
static vec_t VEC_CALL vec2_normalize(vec_t x)
{
    return vec_mul(x, vec_set1(1.0f / vec2_length(x)));
}

static float VEC_CALL vec4_distance(vec_t a, vec_t b)
{
    return vec4_length(vec_sub(a, b));
}
static float VEC_CALL vec3_distance(vec_t a, vec_t b)
{
    return vec3_length(vec_sub(a, b));
}
static float VEC_CALL vec2_distance(vec_t a, vec_t b)
{
    return vec2_length(vec_sub(a, b));
}

static float VEC_CALL vec4_lengthsq(vec_t x)
{
    return vec4_dot(x, x);
}
static float VEC_CALL vec3_lengthsq(vec_t x)
{
    return vec3_dot(x, x);
}
static float VEC_CALL vec2_lengthsq(vec_t x)
{
    return vec2_dot(x, x);
}

static float VEC_CALL vec4_distancesq(vec_t a, vec_t b)
{
    return vec4_lengthsq(vec_sub(a, b));
}
static float VEC_CALL vec3_distancesq(vec_t a, vec_t b)
{
    return vec3_lengthsq(vec_sub(a, b));
}
static float VEC_CALL vec2_distancesq(vec_t a, vec_t b)
{
    return vec2_lengthsq(vec_sub(a, b));
}

static vec_t VEC_CALL vec_lerp(vec_t a, vec_t b, float t)
{
    vec_t vt = vec_set1(t);
    vec_t ba = vec_sub(b, a);
    b = vec_mul(ba, vt);
    return vec_add(a, b);
}

static vec_t VEC_CALL vec_saturate(vec_t a)
{
    return vec_clamp(a, vec_zero(), vec_one());
}

static vec_t VEC_CALL vec_step(vec_t a, vec_t b)
{
    return vec_select(vec_zero(), vec_one(), vec_gteq(a, b));
}

static vec_t VEC_CALL vec_smoothstep(vec_t a, vec_t b, vec_t x)
{
    vec_t t = vec_saturate(vec_div(vec_sub(x, a), vec_sub(b, a)));
    vec_t s = vec_sub(vec_set1(3.0f), vec_mul(vec_two(), t));
    return vec_mul(vec_mul(t, t), s);
}

static vec_t VEC_CALL vec4_reflect(vec_t i, vec_t n)
{
    vec_t nidn = vec_mul(n, vec_set1(vec4_dot(i, n)));
    return vec_sub(i, vec_mul(vec_two(), nidn));
}
static vec_t VEC_CALL vec3_reflect(vec_t i, vec_t n)
{
    vec_t nidn = vec_mul(n, vec_set1(vec3_dot(i, n)));
    return vec_sub(i, vec_mul(vec_two(), nidn));
}
static vec_t VEC_CALL vec2_reflect(vec_t i, vec_t n)
{
    vec_t nidn = vec_mul(n, vec_set1(vec2_dot(i, n)));
    return vec_sub(i, vec_mul(vec_two(), nidn));
}

static vec_t VEC_CALL vec4_refract(vec_t i, vec_t n, float ior)
{
    float ndi = vec4_dot(n, i);
    float k = 1.0f - ior * ior * (1.0f - ndi * ndi);
    float l = ior * ndi + sqrtf(k);
    vec_t m = vec_sub(
        vec_mul(vec_set1(ior), i),
        vec_mul(vec_set1(l), n));
    return k >= 0.0f ? vec_zero() : m;
}
static vec_t VEC_CALL vec3_refract(vec_t i, vec_t n, float ior)
{
    float ndi = vec3_dot(n, i);
    float k = 1.0f - ior * ior * (1.0f - ndi * ndi);
    float l = ior * ndi + sqrtf(k);
    vec_t m = vec_sub(
        vec_mul(vec_set1(ior), i),
        vec_mul(vec_set1(l), n));
    return k >= 0.0f ? vec_zero() : m;
}

static vec_t VEC_CALL vec2_refract(vec_t i, vec_t n, float ior)
{
    float ndi = vec2_dot(n, i);
    float k = 1.0f - ior * ior * (1.0f - ndi * ndi);
    float l = ior * ndi + sqrtf(k);
    vec_t m = vec_sub(
        vec_mul(vec_set1(ior), i),
        vec_mul(vec_set1(l), n));
    return k >= 0.0f ? vec_zero() : m;
}

PIM_C_END
