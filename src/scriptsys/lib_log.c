#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"
#include "scriptsys/script.h"
#include "common/console.h"
#include "lib_log.h"

static int log_sev(lua_State* L, LogSev severity)
{
	lua_concat(L, lua_gettop(L));
	const char* message = luaL_tolstring(L, 1, NULL);
	lua_pop(L, 1);

	Con_Logf(severity, "script", message);
	return 0;
}

static int func_info(lua_State* L)
{
	return log_sev(L, LogSev_Info);
}

static int func_error(lua_State* L)
{
	return log_sev(L, LogSev_Error);
}

static int func_warning(lua_State* L)
{
	return log_sev(L, LogSev_Warning);
}

static int func_verbose(lua_State* L)
{
	return log_sev(L, LogSev_Verbose);
}

void lib_log_init(lua_State* L)
{
	LUA_LIB(L,
		LUA_FN(info),
		LUA_FN(error),
		LUA_FN(warning),
		LUA_FN(verbose));

	LUA_LIB_REG_GLOBAL(L, "Log");
}