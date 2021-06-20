#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"
#include "common/console.h"
#include "common/cmd.h"
#include "common/stringutil.h"
#include "script.h"

static lua_State* L = NULL;

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

	luaL_dofile(L, path);
	return cmdstat_ok;
}

static cmdstat_t cmd_script_eval(i32 argc, const char** argv)
{
	if (argc != 2)
	{
		Con_Logf(LogSev_Error, "script", "script_eval <lua code> : expects a single (quoted) argument, lua code to execute.");
		return cmdstat_err;
	}

	luaL_dostring(L, argv[1]);
	return cmdstat_ok;
}

void init_cmds(lua_State* l)
{
	L = l;

	cmd_reg("script_exec", "<script name>", "executes a script by name in the scripts folder.", cmd_script_exec);
	cmd_reg("script_eval", "\"<lua code>\"", "executes the given argument as a script (surround in quotes).", cmd_script_eval);
}