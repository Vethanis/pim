#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include <stdint.h>

typedef void* thread_t;
typedef int32_t (PIM_CDECL *thread_fn)(void* data);

thread_t thread_create(thread_fn entrypoint, void* data);
void thread_join(thread_t tr);

PIM_C_END
