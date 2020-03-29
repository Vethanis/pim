#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include <stdint.h>

typedef struct semaphore_s { void* handle; } semaphore_t;

void semaphore_create(semaphore_t* sema, int32_t value);
void semaphore_destroy(semaphore_t* sema);
void semaphore_signal(semaphore_t sema, int32_t count);
void semaphore_wait(semaphore_t sema);
int32_t semaphore_trywait(semaphore_t sema);

PIM_C_END
