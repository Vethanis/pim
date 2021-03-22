#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct ProfMark_s
{
    char const *const name;
    u32 hash;
    u32 calls;
    u64 sum;
} ProfMark;

void ProfileSys_Gui(bool* pEnabled);

void _ProfileBegin(ProfMark *const mark);
void _ProfileEnd(ProfMark *const mark);

#define PIM_PROFILE 1

#if PIM_PROFILE
    #define ProfileMark(var, tag)   static ProfMark var = { #tag };
    #define ProfileBegin(mark)      _ProfileBegin(&(mark))
    #define ProfileEnd(mark)        _ProfileEnd(&(mark))
#else
    #define ProfileMark(var, tag)   
    #define ProfileBegin(mark)      (void)0
    #define ProfileEnd(mark)        (void)0
#endif // PIM_PROFILE

PIM_C_END
