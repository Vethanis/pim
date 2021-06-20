#include "scriptsys/script.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"
#include "lua/lua.h"
#include "common/console.h"
#include "common/stringutil.h"
#include "lib_log.h"
#include "lib_time.h"
#include "cmds.h"
#include "lib_game.h"
#include "cmds.h"

static lua_State* L;

void ScriptSys_Init(void)
{
	L = luaL_newstate();
	luaL_openlibs(L);

	init_cmds(L);
	lib_log_init(L);
	lib_time_init(L);
	lib_game_init(L);

	ScriptSys_Exec("init");
}

void ScriptSys_Shutdown(void)
{
	lib_game_shutdown(L);

	lua_close(L);
	L = NULL;
}

void ScriptSys_Update(void)
{
	if (lib_game_num_scripts() <= 0)
	{
		return;
	}

	lib_time_update(L);
	lib_game_update(L);
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
		Con_Logf(LogSev_Error, "script", "path is too long (> %i)", PIM_PATH);
		return false;
	}

	Con_Logf(LogSev_Verbose, "script", "executing script from %s", path);

	if (luaL_dofile(L, path) != LUA_OK)
	{
		Con_Logf(LogSev_Error, "script", lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}

	return true;
}

bool ScriptSys_Eval(const char* script)
{
	if (luaL_dostring(L, script) != LUA_OK)
	{
		Con_Logf(LogSev_Error, "script", lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}

	return true;
}


