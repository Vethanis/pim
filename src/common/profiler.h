#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct profmrk_s
{
    const char* name;
    u64 sum;
    u64 calls;
    double average;
} profmark_t;

typedef struct profinfo_s
{
    profmark_t* marks;
    i32 length;
} profinfo_t;

void profile_sys_init(void);
void profile_sys_gui(void);
void profile_sys_collect(void);
void profile_sys_shutdown(void);

void _ProfileBegin(profmark_t* mark);
void _ProfileEnd(profmark_t* mark);

const profinfo_t* ProfileInfo(void);

#define PIM_PROFILE 1

#if PIM_PROFILE
    #define ProfileMark(var, tag)   static profmark_t var = { #tag };
    #define ProfileBegin(mark)      _ProfileBegin(&(mark))
    #define ProfileEnd(mark)        _ProfileEnd(&(mark))
#else
    #define ProfileMark(var)        
    #define ProfileBegin(mark)      (void)0
    #define ProfileEnd(mark)        (void)0
#endif // PIM_PROFILE

PIM_C_END
