#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"
#include "scriptsys/script.h"
#include "common/console.h"
#include "containers/hash_set.h"
#include "allocator/allocator.h"
#include "scr_log.h"
#include "scr_game.h"

static HashSet sm_update_handlers;

static int scr_func_start_update(lua_State* L)
{
	lua_pushvalue(L, 1);
	i32 refId = luaL_ref(L, LUA_REGISTRYINDEX);
	if (refId == LUA_REFNIL)
	{
		return 0;
	}

	Con_Logf(LogSev_Verbose, "script", "starting script activity #%i", refId);

	if (lua_getfield(L, 1, "start") != LUA_TNIL)
	{
		lua_pushvalue(L, 1); // self arg
		if (lua_pcall(L, 1, 0, 0) != LUA_OK)
		{
			Con_Logf(LogSev_Error, "script", lua_tostring(L, -1));
			lua_pop(L, 1); // error
			return 0;
		}
	}

	lua_pushinteger(L, refId);
	lua_setfield(L, 1, "__update_ref"); // set table.__update_ref for later unregistering
	HashSet_Add(&sm_update_handlers, &refId, sizeof(i32));

	return 0;
}

static void scr_remove_update_handler(lua_State* L, i32 refId)
{
	Con_Logf(LogSev_Verbose, "script", "stopping script activity #%i", refId);

	lua_rawgeti(L, LUA_REGISTRYINDEX, refId);
	if (lua_getfield(L, 1, "stop") != LUA_TNIL)
	{
		lua_pushvalue(L, 1); // self arg
		if (lua_pcall(L, 1, 0, 0) != LUA_OK)
		{
			Con_Logf(LogSev_Error, "script", lua_tostring(L, -1));
			lua_pop(L, 1); // error
		}
	}
	lua_pop(L, 1);

	HashSet_Rm(&sm_update_handlers, &refId, sizeof(i32));
	luaL_unref(L, LUA_REGISTRYINDEX, refId);
}

static int scr_func_stop_update(lua_State* L)
{
	lua_getfield(L, 1, "__update_ref");
	i32 refId = (i32)lua_tointeger(L, 2);

	scr_remove_update_handler(L, refId);
	return 0;
}

void scr_game_init(lua_State* L)
{
	LUA_LIB(L,
		LUA_FN(start_update),
		LUA_FN(stop_update));

	Script_RegisterLib(L, "Game", ScrLib_Global);

	HashSet_New(&sm_update_handlers, sizeof(i32), EAlloc_Perm);
}

void scr_game_shutdown(lua_State* L)
{
	i32* keys = sm_update_handlers.keys;
	for (u32 i = 0; i < sm_update_handlers.width; i++)
	{
		i32 refId = keys[i];
		if (!refId)
		{
			continue;
		}

		scr_remove_update_handler(L, refId);
	}

	HashSet_Del(&sm_update_handlers);
}

void scr_game_update(lua_State* L)
{
	i32* keys = sm_update_handlers.keys;

	for (u32 i = 0; i < sm_update_handlers.width; i++)
	{
		i32 refId = keys[i];
		if (!refId)
		{
			continue;
		}

		lua_rawgeti(L, LUA_REGISTRYINDEX, refId);
		lua_getfield(L, 1, "update");
		lua_pushvalue(L, 1); // self arg
		if (lua_pcall(L, 1, 0, 0) != LUA_OK)
		{
			Con_Logf(LogSev_Error, "script", lua_tostring(L, -1));
			lua_pop(L, 1); // error
			scr_remove_update_handler(L, refId);
		}

		lua_pop(L, 1); // table
	}
}

i32 scr_game_num_scripts(void)
{
	return sm_update_handlers.count;
}
