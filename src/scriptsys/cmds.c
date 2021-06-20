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

	return ScriptSys_Exec(argv[1]) ? cmdstat_ok : cmdstat_err;
}

static cmdstat_t cmd_script_eval(i32 argc, const char** argv)
{
	if (argc != 2)
	{
		Con_Logf(LogSev_Error, "script", "script_eval <lua code> : expects a single (quoted) argument, lua code to execute.");
		return cmdstat_err;
	}

	return ScriptSys_Eval(argv[1]) ? cmdstat_ok : cmdstat_err;
}

void init_cmds(lua_State* l)
{
	L = l;

	cmd_reg("script_exec", "<script name>", "executes a script by name in the scripts folder.", cmd_script_exec);
	cmd_reg("script_eval", "\"<lua code>\"", "executes the given argument as a script (surround in quotes).", cmd_script_eval);
}