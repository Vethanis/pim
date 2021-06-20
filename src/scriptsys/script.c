#include "scriptsys/script.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"
#include "lua/lua.h"
#include "common/console.h"
#include "common/stringutil.h"
#include "scr_log.h"
#include "scr_time.h"
#include "scr_cmd.h"
#include "scr_game.h"

static lua_State* L;

void ScriptSys_Init(void)
{
	L = luaL_newstate();
	luaL_openlibs(L);

	scr_cmd_init(L);
	scr_log_init(L);
	scr_time_init(L);
	scr_game_init(L);

	ScriptSys_Exec("init");
}

void ScriptSys_Shutdown(void)
{
	scr_game_shutdown(L);

	lua_close(L);
	L = NULL;
}

void ScriptSys_Update(void)
{
	if (scr_game_num_scripts() <= 0)
	{
		return;
	}

	scr_time_update(L);
	scr_game_update(L);
}

void Script_RegisterLib(lua_State* L, const char* name, ScrLib_Reg regType)
{
	if (regType == ScrLib_Global)
	{
		lua_setglobal(L, name);
		return;
	}

	lua_getglobal(L, "package");
	lua_getfield(L, -1, "loaded");
	lua_pushvalue(L, 1);
	lua_setfield(L, -2, name);
	lua_pop(L, 3);
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


