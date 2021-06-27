#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct lua_State lua_State;

void scr_time_init(lua_State* L);
void scr_time_update(lua_State* L);
void scr_time_shutdown(lua_State* L);

PIM_C_END
