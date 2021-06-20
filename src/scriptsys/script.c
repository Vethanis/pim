#include "scriptsys/script.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"
#include "lua/lua.h"
#include "common/stringutil.h"
#include "common/console.h"
#include "common/cmd.h"

static lua_State* ms_luastate;

static int lib_console_log(lua_State* L)
{
	lua_concat(L, lua_gettop(L));
	const char* message = luaL_tolstring(L, 1, NULL);
	lua_pop(L, 1);

	Con_Logf(LogSev_Info, "script", message);
	return 0;
}

static cmdstat_t cmd_script_exec(i32 argc, const char** argv)
{
	if (argc != 2)
	{
		Con_Logf(LogSev_Error, "script", "script_exec <file> : expects a single argument, a relative path in the script directory.");
		return cmdstat_err;
	}

	char path[PIM_PATH];
	StrCpy(ARGS(path), SCRIPT_DIR);
	i32 len = StrCat(ARGS(path), argv[1]);

	if (!IEndsWith(ARGS(path), ".lua"))
	{
		len = StrCat(ARGS(path), ".lua");
	}

	if (len > PIM_PATH)
	{
		Con_Logf(LogSev_Error, "script", "Path is too long (> %i)", PIM_PATH);
		return cmdstat_err;
	}

	luaL_dofile(ms_luastate, path);
	return cmdstat_ok;
}

static cmdstat_t cmd_script_eval(i32 argc, const char** argv)
{
	if (argc != 2)
	{
		Con_Logf(LogSev_Error, "script", "script_eval <lua code> : expects a single (quoted) argument, lua code to execute.");
		return cmdstat_err;
	}

	luaL_dostring(ms_luastate, argv[1]);
	return cmdstat_ok;
}

void ScriptSys_Init()
{
	cmd_reg("script_exec", "<script name>", "executes a script by name in the scripts folder.", cmd_script_exec);
	cmd_reg("script_eval", "\"<lua code>\"", "executes the given argument as a script (surround in quotes).", cmd_script_eval);

	ms_luastate = luaL_newstate();
	luaL_openlibs(ms_luastate);

	luaL_Reg console_lib[] =
	{
		{
			.name = "log",
			.func = lib_console_log
		},
		{ 0 }
	};

	luaL_newlib(ms_luastate, console_lib);
	lua_setglobal(ms_luastate, "Console");

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


