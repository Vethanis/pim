#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "components/components.h"

u32 compflag_create(i32 count, ...);

static bool compflag_all(u32 has, u32 all)
{
    return (has & all) == all;
}

static bool compflag_any(u32 has, u32 any)
{
    return (has & any) != 0u;
}

static bool compflag_none(u32 has, u32 none)
{
    return (has & none) == 0u;
}

static bool compflag_get(u32 flag, compid_t id)
{
    return (flag & (1u << id)) != 0;
}

static u32 compflag_set(u32 flag, compid_t id)
{
    return flag | (1u << id);
}

static u32 compflag_unset(u32 flag, compid_t id)
{
    return flag & ~(1u << id);
}

PIM_C_END
