#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct text16_s
{
    char c[16];
} text16;

typedef struct text32_s
{
    char c[32];
} text32;

typedef struct text64_s
{
    char c[64];
} text64;

typedef struct text128_s
{
    char c[128];
} text128;

typedef struct text256_s
{
    char c[256];
} text256;

typedef struct text512_s
{
    char c[512];
} text512;

typedef struct text1k_s
{
    char c[1024];
} text1k;

typedef struct text2k_s
{
    char c[2048];
} text2k;

typedef struct text4k_s
{
    char c[4096];
} text4k;

void text_new(void* txt, i32 sizeOf, const char* str);

PIM_C_END
