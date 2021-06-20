#pragma once
#include "common/macro.h"

#define SCRIPT_DIR "script/"

#define LUA_FN(n) { .name = #n, .func = func_##n }

#define LUA_LIB(L, ...) luaL_Reg lib[] = \
{ \
	__VA_ARGS__, \
	{ 0 } \
}; \
luaL_newlib(L, lib);

#define LUA_LIB_EMPTY(L) luaL_Reg lib[] = { { 0 } }; \
luaL_newlib(L, lib);

#define LUA_LIB_REG_GLOBAL(L, name) lua_setglobal(L, name);

#define LUA_LIB_REG_IMPORT(L, name) lua_getglobal(L, "package"); \
	lua_getfield(L, -1, "loaded"); \
	lua_pushvalue(L, 1); \
	lua_setfield(L, -2, name); \
	lua_pop(L, 3);

PIM_C_BEGIN

typedef struct lua_State lua_State;

void ScriptSys_Init(void);
void ScriptSys_Shutdown(void);
void ScriptSys_Update(void);

bool ScriptSys_Exec(const char* filename);
bool ScriptSys_Eval(const char* filename);

PIM_C_END
