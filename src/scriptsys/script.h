#pragma once

#include "lua/lua.h"

#define SCRIPT_DIR "script/"

static lua_State* ms_luastate;

void ScriptSys_Init();
void ScriptSys_Shutdown();