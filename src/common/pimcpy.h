#pragma once

#include "common/macro.h"

PIM_C_BEGIN

void pimcpy(
    void* pim_noalias dst,
    const void* pim_noalias src,
    i32 len);

void pimmove(void* dst, const void* src, i32 len);

void pimset(void* dst, u32 val, i32 len);

i32 pimcmp(
    const void* pim_noalias lhs,
    const void* pim_noalias rhs,
    i32 len);

PIM_C_END
