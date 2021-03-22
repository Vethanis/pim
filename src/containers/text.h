#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct Text16_s
{
    char c[16];
} Text16;

typedef struct Text32_s
{
    char c[32];
} Text32;

typedef struct Text64_s
{
    char c[64];
} Text64;

typedef struct Text128_s
{
    char c[128];
} Text128;

typedef struct Text256_s
{
    char c[256];
} Text256;

typedef struct Text512_s
{
    char c[512];
} Text512;

typedef struct Text1k_s
{
    char c[1024];
} Text1k;

typedef struct Text2k_s
{
    char c[2048];
} Text2k;

typedef struct Text4k_s
{
    char c[4096];
} Text4k;

void Text_New(void* txt, i32 sizeOf, const char* str);

PIM_C_END
