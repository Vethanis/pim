#pragma once
#include "common/macro.h"

#define SCRIPT_DIR "script/"

#define LUA_FN(n) { .name = #n, .func = func_##n }

PIM_C_BEGIN

typedef struct lua_State lua_State;

void ScriptSys_Init();
void ScriptSys_Shutdown();
void ScriptSys_Update();

PIM_C_END
