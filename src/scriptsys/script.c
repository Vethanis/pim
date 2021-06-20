#include "scriptsys/script.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"

static lua_State* ms_luastate;

void ScriptSys_Init()
{
	ms_luastate = luaL_newstate();
	luaL_openlibs(ms_luastate);

	luaL_dofile(ms_luastate, SCRIPT_DIR "init.lua");
}

void ScriptSys_Shutdown()
{
	lua_close(ms_luastate);
	ms_luastate = NULL;
}

void ScriptSys_Update()
{
}
