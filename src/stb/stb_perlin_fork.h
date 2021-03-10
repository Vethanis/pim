// stb_perlin.h - v0.5 - perlin noise
// public domain single-file C implementation by Sean Barrett
//
// !!! MODIFIED FROM ORIGINAL !!!
//
// LICENSE
//
//   See end of file.
//
//
// to create the implementation,
//     #define STB_PERLIN_IMPLEMENTATION
// in *one* C/CPP file that includes this file.
//
//
// Documentation:
//
// float  stb_perlin_noise3( float x,
//                           float y,
//                           float z,
//                           int   x_wrap=0,
//                           int   y_wrap=0,
//                           int   z_wrap=0)
//
// This function computes a random value at the coordinate (x,y,z).
// Adjacent random values are continuous but the noise fluctuates
// its randomness with period 1, i.e. takes on wholly unrelated values
// at integer points. Specifically, this implements Ken Perlin's
// revised noise function from 2002.
//
// The "wrap" parameters can be used to create wraparound noise that
// wraps at powers of two. The numbers MUST be powers of two. Specify
// 0 to mean "don't care". (The noise always wraps every 256 due
// details of the implementation, even if you ask for larger or no
// wrapping.)
//
// float  stb_perlin_noise3_seed( float x,
//                                float y,
//                                float z,
//                                int   x_wrap=0,
//                                int   y_wrap=0,
//                                int   z_wrap=0,
//                                int   seed)
//
// As above, but 'seed' selects from multiple different variations of the
// noise function. The current implementation only uses the bottom 8 bits
// of 'seed', but possibly in the future more bits will be used.
//
//
// Fractal Noise:
//
// Three common fractal noise functions are included, which produce
// a wide variety of nice effects depending on the parameters
// provided. Note that each function will call stb_perlin_noise3
// 'octaves' times, so this parameter will affect runtime.
//
// float stb_perlin_ridge_noise3(float x, float y, float z,
//                               float lacunarity, float gain, float offset, int octaves)
//
// float stb_perlin_fbm_noise3(float x, float y, float z,
//                             float lacunarity, float gain, int octaves)
//
// float stb_perlin_turbulence_noise3(float x, float y, float z,
//                                    float lacunarity, float gain, int octaves)
//
// Typical values to start playing with:
//     octaves    =   6     -- number of "octaves" of noise3() to sum
//     lacunarity = ~ 2.0   -- spacing between successive octaves (use exactly 2.0 for wrapping output)
//     gain       =   0.5   -- relative weighting applied to each successive octave
//     offset     =   1.0?  -- used to invert the ridges, may need to be larger, not sure
//
//
// Contributors:
//    Jack Mott - additional noise functions
//    Jordan Peck - seeded noise
//

#include "common/macro.h"
#include "math/scalar.h"

// not same permutation table as Perlin's reference to avoid copyright issues;
// Perlin's table can be found at http://mrl.nyu.edu/~perlin/noise/
static const u8 stb__perlinf_randtab[256] =
{
   23, 125, 161, 52, 103, 117, 70, 37, 247, 101, 203, 169, 124, 126, 44, 123,
   152, 238, 145, 45, 171, 114, 253, 10, 192, 136, 4, 157, 249, 30, 35, 72,
   175, 63, 77, 90, 181, 16, 96, 111, 133, 104, 75, 162, 93, 56, 66, 240,
   8, 50, 84, 229, 49, 210, 173, 239, 141, 1, 87, 18, 2, 198, 143, 57,
   225, 160, 58, 217, 168, 206, 245, 204, 199, 6, 73, 60, 20, 230, 211, 233,
   94, 200, 88, 9, 74, 155, 33, 15, 219, 130, 226, 202, 83, 236, 42, 172,
   165, 218, 55, 222, 46, 107, 98, 154, 109, 67, 196, 178, 127, 158, 13, 243,
   65, 79, 166, 248, 25, 224, 115, 80, 68, 51, 184, 128, 232, 208, 151, 122,
   26, 212, 105, 43, 179, 213, 235, 148, 146, 89, 14, 195, 28, 78, 112, 76,
   250, 47, 24, 251, 140, 108, 186, 190, 228, 170, 183, 139, 39, 188, 244, 246,
   132, 48, 119, 144, 180, 138, 134, 193, 82, 182, 120, 121, 86, 220, 209, 3,
   91, 241, 149, 85, 205, 150, 113, 216, 31, 100, 41, 164, 177, 214, 153, 231,
   38, 71, 185, 174, 97, 201, 29, 95, 7, 92, 54, 254, 191, 118, 34, 221,
   131, 11, 163, 99, 234, 81, 227, 147, 156, 176, 17, 142, 69, 12, 110, 62,
   27, 255, 0, 194, 59, 116, 242, 252, 19, 21, 187, 53, 207, 129, 64, 135,
   61, 40, 167, 237, 102, 223, 106, 159, 197, 189, 215, 137, 36, 32, 22, 5,
};
pim_inline u8 VEC_CALL stb__perlinf_sample_randtab(u32 i)
{
    return stb__perlinf_randtab[i & 0xff];
}

// perlin's gradient has 12 cases so some get used 1/16th of the time
// and some 2/16ths. We reduce bias by changing those fractions
// to 5/64ths and 6/64ths

// this array is designed to match the previous implementation
// of gradient hash: indices[stb__perlin_randtab[i]&63]
static const u8 stb__perlinf_randtab_grad_idx[256] =
{
    7, 9, 5, 0, 11, 1, 6, 9, 3, 9, 11, 1, 8, 10, 4, 7,
    8, 6, 1, 5, 3, 10, 9, 10, 0, 8, 4, 1, 5, 2, 7, 8,
    7, 11, 9, 10, 1, 0, 4, 7, 5, 0, 11, 6, 1, 4, 2, 8,
    8, 10, 4, 9, 9, 2, 5, 7, 9, 1, 7, 2, 2, 6, 11, 5,
    5, 4, 6, 9, 0, 1, 1, 0, 7, 6, 9, 8, 4, 10, 3, 1,
    2, 8, 8, 9, 10, 11, 5, 11, 11, 2, 6, 10, 3, 4, 2, 4,
    9, 10, 3, 2, 6, 3, 6, 10, 5, 3, 4, 10, 11, 2, 9, 11,
    1, 11, 10, 4, 9, 4, 11, 0, 4, 11, 4, 0, 0, 0, 7, 6,
    10, 4, 1, 3, 11, 5, 3, 4, 2, 9, 1, 3, 0, 1, 8, 0,
    6, 7, 8, 7, 0, 4, 6, 10, 8, 2, 3, 11, 11, 8, 0, 2,
    4, 8, 3, 0, 0, 10, 6, 1, 2, 2, 4, 5, 6, 0, 1, 3,
    11, 9, 5, 5, 9, 6, 9, 8, 3, 8, 1, 8, 9, 6, 9, 11,
    10, 7, 5, 6, 5, 9, 1, 3, 7, 0, 2, 10, 11, 2, 6, 1,
    3, 11, 7, 7, 2, 1, 7, 3, 0, 8, 1, 1, 5, 0, 6, 10,
    11, 11, 0, 2, 7, 0, 10, 8, 3, 5, 7, 1, 11, 1, 0, 7,
    9, 0, 11, 5, 10, 3, 2, 3, 5, 9, 7, 9, 8, 4, 6, 5,
};

static const float stb__perlinf_basis[12][3] =
{
   {  1.0f, 1.0f, 0.0f, },
   { -1.0f, 1.0f, 0.0f, },
   {  1.0f,-1.0f, 0.0f, },
   { -1.0f,-1.0f, 0.0f, },
   {  1.0f, 0.0f, 1.0f, },
   { -1.0f, 0.0f, 1.0f, },
   {  1.0f, 0.0f,-1.0f, },
   { -1.0f, 0.0f,-1.0f, },
   {  0.0f, 1.0f, 1.0f, },
   {  0.0f,-1.0f, 1.0f, },
   {  0.0f, 1.0f,-1.0f, },
   {  0.0f,-1.0f,-1.0f, },
};

// different grad function from Perlin's, but easy to modify to match reference
pim_inline float VEC_CALL stb__perlinf_grad(u32 idx, float x, float y, float z)
{
    u8 grad_idx = stb__perlinf_randtab_grad_idx[idx & 0xff];
    float gx = stb__perlinf_basis[grad_idx][0];
    float gy = stb__perlinf_basis[grad_idx][1];
    float gz = stb__perlinf_basis[grad_idx][2];
    return gx * x + gy * y + gz * z;
}

pim_inline float VEC_CALL stb_perlinf_noise3_internal(float x, float y, float z, u8 seed)
{
    i32 px = (i32)f1_floor(x);
    i32 py = (i32)f1_floor(y);
    i32 pz = (i32)f1_floor(z);

    u8 x0 = px & 0xff; u8 x1 = (px + 1) & 0xff;
    u8 y0 = py & 0xff; u8 y1 = (py + 1) & 0xff;
    u8 z0 = pz & 0xff; u8 z1 = (pz + 1) & 0xff;

    x -= px;
    y -= py;
    z -= pz;

    u8 r0 = stb__perlinf_sample_randtab(x0 + seed);
    u8 r1 = stb__perlinf_sample_randtab(x1 + seed);

    u8 r00 = stb__perlinf_sample_randtab(r0 + y0);
    u8 r01 = stb__perlinf_sample_randtab(r0 + y1);
    u8 r10 = stb__perlinf_sample_randtab(r1 + y0);
    u8 r11 = stb__perlinf_sample_randtab(r1 + y1);

    float n000 = stb__perlinf_grad(r00 + z0, x, y, z);
    float n001 = stb__perlinf_grad(r00 + z1, x, y, z - 1.0f);
    float n010 = stb__perlinf_grad(r01 + z0, x, y - 1.0f, z);
    float n011 = stb__perlinf_grad(r01 + z1, x, y - 1.0f, z - 1.0f);
    float n100 = stb__perlinf_grad(r10 + z0, x - 1.0f, y, z);
    float n101 = stb__perlinf_grad(r10 + z1, x - 1.0f, y, z - 1.0f);
    float n110 = stb__perlinf_grad(r11 + z0, x - 1.0f, y - 1.0f, z);
    float n111 = stb__perlinf_grad(r11 + z1, x - 1.0f, y - 1.0f, z - 1.0f);

    float u = f1_unormerstep(x);
    float v = f1_unormerstep(y);
    float w = f1_unormerstep(z);

    float n00 = f1_lerp(n000, n001, w);
    float n01 = f1_lerp(n010, n011, w);
    float n10 = f1_lerp(n100, n101, w);
    float n11 = f1_lerp(n110, n111, w);

    float n0 = f1_lerp(n00, n01, v);
    float n1 = f1_lerp(n10, n11, v);

    return f1_lerp(n0, n1, u);
}

pim_inline float VEC_CALL stb_perlinf_noise3(float x, float y, float z)
{
    return stb_perlinf_noise3_internal(x, y, z, 0);
}

pim_inline float VEC_CALL stb_perlinf_noise3_seed(float x, float y, float z, u8 seed)
{
    return stb_perlinf_noise3_internal(x, y, z, seed);
}

pim_inline float VEC_CALL stb_perlinf_ridge_noise3(float x, float y, float z, float lacunarity, float gain, float offset, u8 octaves)
{
    float frequency = 1.0f;
    float prev = 1.0f;
    float amplitude = 0.5f;
    float sum = 0.0f;
    for (u8 i = 0; i < octaves; ++i)
    {
        float r = stb_perlinf_noise3_internal(x*frequency, y*frequency, z*frequency, i);
        r = offset - f1_abs(r);
        r = r * r;
        sum += r * amplitude*prev;
        prev = r;
        frequency *= lacunarity;
        amplitude *= gain;
    }
    return sum;
}

pim_inline float VEC_CALL stb_perlinf_fbm_noise3(float x, float y, float z, float lacunarity, float gain, u8 octaves)
{
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float sum = 0.0f;
    for (u8 i = 0; i < octaves; ++i)
    {
        sum += stb_perlinf_noise3_internal(x*frequency, y*frequency, z*frequency, i) * amplitude;
        frequency *= lacunarity;
        amplitude *= gain;
    }
    return sum;
}

pim_inline float VEC_CALL stb_perlinf_turbulence_noise3(float x, float y, float z, float lacunarity, float gain, u8 octaves)
{
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float sum = 0.0f;
    for (u8 i = 0; i < octaves; ++i)
    {
        float r = stb_perlinf_noise3_internal(x*frequency, y*frequency, z*frequency, i) * amplitude;
        sum += f1_abs(r);
        frequency *= lacunarity;
        amplitude *= gain;
    }
    return sum;
}

/*
------------------------------------------------------------------------------
This software is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2017 Sean Barrett
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------
*/
