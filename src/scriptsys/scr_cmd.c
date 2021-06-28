#include "scriptsys/scr_cmd.h"

#include "scriptsys/script.h"

#include "common/console.h"
#include "common/cmd.h"
#include "common/stringutil.h"

#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

// ----------------------------------------------------------------------------

static i32 scr_func_exec(lua_State* L);

static cmdstat_t scr_cmd_exec(i32 argc, const char** argv);
static cmdstat_t scr_cmd_eval(i32 argc, const char** argv);

// ----------------------------------------------------------------------------

static const luaL_Reg ms_regs[] =
{
    {.name = "exec",.func = scr_func_exec },
    {0},
};

// ----------------------------------------------------------------------------

void scr_cmd_init(lua_State* L)
{
    cmd_reg("script_exec", "<script name>", "executes a script by name in the scripts folder.", scr_cmd_exec);
    cmd_reg("script_eval", "\"<lua code>\"", "executes the given argument as a script (surround in quotes).", scr_cmd_eval);

    luaL_newlib(L, ms_regs);
    lua_pushinteger(L, cmdstat_ok);
    lua_setfield(L, 1, "OK");
    lua_pushinteger(L, cmdstat_err);
    lua_setfield(L, 1, "ERR");
    Script_RegisterLib(L, "cmd", ScrLib_Import);
}

void scr_cmd_shutdown(lua_State* L)
{

}

// ----------------------------------------------------------------------------

static i32 scr_func_exec(lua_State* L)
{
    const char* line = luaL_checkstring(L, 1);

    cmdstat_t result = cmd_enqueue(line);
    lua_pushinteger(L, result);
    return 1;
}

static cmdstat_t scr_cmd_exec(i32 argc, const char** argv)
{
    if (argc != 2)
    {
        Con_Logf(LogSev_Error, "scr", "script_exec <file> : expects a single argument, a relative path in the script directory.");
        return cmdstat_err;
    }

    return ScriptSys_Exec(argv[1]) ? cmdstat_ok : cmdstat_err;
}

static cmdstat_t scr_cmd_eval(i32 argc, const char** argv)
{
    if (argc != 2)
    {
        Con_Logf(LogSev_Error, "scr", "script_eval <lua code> : expects a single (quoted) argument, lua code to execute.");
        return cmdstat_err;
    }

    return ScriptSys_Eval(argv[1]) ? cmdstat_ok : cmdstat_err;
}
