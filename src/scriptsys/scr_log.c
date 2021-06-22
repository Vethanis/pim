#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"
#include "scriptsys/script.h"
#include "common/console.h"
#include "scr_log.h"

static int scr_log_sev(lua_State* L, LogSev severity)
{
	lua_concat(L, lua_gettop(L));
	const char* message = luaL_tolstring(L, 1, NULL);
	lua_pop(L, 1);

	Con_Logf(severity, "script", message);
	return 0;
}

static int scr_func_info(lua_State* L)
{
	return scr_log_sev(L, LogSev_Info);
}

static int scr_func_error(lua_State* L)
{
	return scr_log_sev(L, LogSev_Error);
}

static int scr_func_warning(lua_State* L)
{
	return scr_log_sev(L, LogSev_Warning);
}

static int scr_func_verbose(lua_State* L)
{
	return scr_log_sev(L, LogSev_Verbose);
}

void scr_log_init(lua_State* L)
{
	LUA_LIB(L,
		LUA_FN(info),
		LUA_FN(error),
		LUA_FN(warning),
		LUA_FN(verbose));

	Script_RegisterLib(L, "Log", ScrLib_Global);
}

void scr_log_shutdown(lua_State* L)
{
}
