#include "scriptsys/script.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"
#include "lua/lua.h"
#include "lib_log.h"
#include "cmds.h"

static lua_State* L;

void ScriptSys_Init(void)
{
	L = luaL_newstate();
	luaL_openlibs(L);

	init_cmds(L);
	init_lib_log(L);
	luaL_dofile(L, SCRIPT_DIR "init.lua");
}

void ScriptSys_Shutdown(void)
{
	lua_close(L);
	L = NULL;
}

void ScriptSys_Update(void)
{
}


