#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct lua_State lua_State;

void scr_cvar_init(lua_State* L);
void scr_cvar_shutdown(lua_State* L);

PIM_C_END
