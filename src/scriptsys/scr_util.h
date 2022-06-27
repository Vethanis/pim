#pragma once

#include <common/macro.h>

PIM_C_BEGIN

typedef struct lua_State lua_State;

float scr_checknumberfield(lua_State* L, i32 pos, const char* field);

PIM_C_END
