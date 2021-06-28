#pragma once

#include "common/macro.h"

#define SCRIPT_DIR "script/"

#define LUA_FN(n) { .name = #n, .func = scr##_func_##n }

#define LUA_LIB(L, ...) { luaL_Reg lib[] = \
{ \
	__VA_ARGS__, \
	{ 0 } \
}; \
luaL_newlib(L, lib); }

#define LUA_LIB_EMPTY(L) luaL_Reg lib[] = { { 0 } }; \
luaL_newlib(L, lib);

PIM_C_BEGIN

typedef struct lua_State lua_State;

typedef enum
{
    ScrLib_Global,
    ScrLib_Import,

    ScrLib_COUNT
} ScrLib_Reg;

void ScriptSys_Init(void);
void ScriptSys_Shutdown(void);
void ScriptSys_Update(void);

void Script_RegisterLib(lua_State* L, const char* name, ScrLib_Reg regType);

bool ScriptSys_Exec(const char* filename);
bool ScriptSys_Eval(const char* expression);

PIM_C_END
