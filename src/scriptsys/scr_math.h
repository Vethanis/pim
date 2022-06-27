#pragma once

#include "common/macro.h"
#include "math/types.h"

PIM_C_BEGIN

typedef struct lua_State lua_State;

void scr_math_init(lua_State* L);
void scr_math_shutdown(lua_State* L);
void VEC_CALL scr_push_f4(lua_State* L, float4 vec);
float4 scr_check_f4(lua_State* L, i32 pos);

PIM_C_END
