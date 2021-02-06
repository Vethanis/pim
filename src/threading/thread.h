#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef enum
{
    thread_priority_lowest = -2,
    thread_priority_lower = -1,
    thread_priority_normal = 0,
    thread_priority_higher = 1,
    thread_priority_highest = 2,
} thread_priority_t;

typedef struct thread_s { void* handle; } thread_t;
typedef i32 (PIM_CDECL *thread_fn)(void* data);

void thread_create(thread_t* tr, thread_fn entrypoint, void* data);
void thread_join(thread_t* tr);
void thread_set_aff(thread_t* tr, u64 mask);
void thread_set_priority(thread_t* tr, thread_priority_t priority);
i32 thread_hardware_count(void);

PIM_C_END
