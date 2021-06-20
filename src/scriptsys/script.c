#include "scriptsys/script.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"
#include "lua/lua.h"
#include "common/console.h"
#include "common/stringutil.h"
#include "lib_log.h"
#include "lib_time.h"
#include "cmds.h"

static lua_State* L;

void ScriptSys_Init(void)
{
	L = luaL_newstate();
	luaL_openlibs(L);

	init_cmds(L);
	init_lib_log(L);
	init_lib_time(L);

	ScriptSys_Exec("init");
}

void ScriptSys_Shutdown(void)
{
	lua_close(L);
	L = NULL;
}

void ScriptSys_Update(void)
{
}

bool ScriptSys_Exec(const char* filename)
{
	char path[PIM_PATH];
	StrCpy(ARGS(path), SCRIPT_DIR);
	i32 len = StrCat(ARGS(path), filename);
	if (!IEndsWith(ARGS(path), ".lua"))
	{
		len = StrCat(ARGS(path), ".lua");
	}

	if (len > PIM_PATH)
	{
		Con_Logf(LogSev_Error, "script", "Path is too long (> %i)", PIM_PATH);
		return false;
	}

	if (luaL_dofile(L, path) != LUA_OK)
	{
		Con_Logf(LogSev_Error, "script", lua_tostring(L, -1));
		return false;
	}

	return true;
}

bool ScriptSys_Eval(const char* script)
{
	if (luaL_dostring(L, script) != LUA_OK)
	{
		Con_Logf(LogSev_Error, "script", lua_tostring(L, -1));
		return false;
	}

	return true;
}


