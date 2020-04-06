#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct semaphore_s { void* handle; } semaphore_t;

void semaphore_create(semaphore_t* sema, i32 value);
void semaphore_destroy(semaphore_t* sema);
void semaphore_signal(semaphore_t sema, i32 count);
void semaphore_wait(semaphore_t sema);
bool semaphore_trywait(semaphore_t sema);

PIM_C_END
