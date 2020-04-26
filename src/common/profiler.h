#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct profmrk_s
{
    const char* name;
    u32 hash;
    u32 calls;
    u64 sum;
} profmark_t;

void profile_gui(bool* pEnabled);

void _ProfileBegin(profmark_t* mark);
void _ProfileEnd(profmark_t* mark);

#define PIM_PROFILE 1

#if PIM_PROFILE
    #define ProfileMark(var, tag)   static profmark_t var = { #tag };
    #define ProfileBegin(mark)      _ProfileBegin(&(mark))
    #define ProfileEnd(mark)        _ProfileEnd(&(mark))
#else
    #define ProfileMark(var, tag)   
    #define ProfileBegin(mark)      (void)0
    #define ProfileEnd(mark)        (void)0
#endif // PIM_PROFILE

PIM_C_END
