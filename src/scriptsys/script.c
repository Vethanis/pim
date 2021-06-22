#include "scriptsys/script.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"
#include "lua/lua.h"
#include "common/console.h"
#include "common/stringutil.h"
#include "scr_log.h"
#include "scr_time.h"
#include "scr_cmd.h"
#include "scr_cvar.h"
#include "scr_game.h"
#include "common/profiler.h"
#include "allocator/allocator.h"

static lua_State* L;


static void* scr_lua_alloc(void* ud, void* ptr, size_t osize, size_t nsize)
{
	if (nsize == 0)
	{
		Mem_Free(ptr);
		return NULL;
	}
	else
	{
		ASSERT((i32)nsize > 0);
		return Mem_Realloc(EAlloc_Script, ptr, (i32)nsize);
	}
}

void ScriptSys_Init(void)
{
	L = lua_newstate(scr_lua_alloc, NULL);
	ASSERT(L);

	luaL_openlibs(L);

	scr_cmd_init(L);
	scr_log_init(L);
	scr_time_init(L);
	scr_cvar_init(L);
	scr_game_init(L);

	ScriptSys_Exec("init");
}

void ScriptSys_Shutdown(void)
{
	scr_game_shutdown(L);
	scr_cmd_shutdown(L);
	scr_log_shutdown(L);
	scr_time_shutdown(L);
	scr_cvar_shutdown(L);

	lua_close(L);
	L = NULL;
}

ProfileMark(pm_update, scrUpdate)
ProfileMark(pm_time_update, scrUpdate_Time)
ProfileMark(pm_game_update, scrUpdate_Game)
void ScriptSys_Update(void)
{
	if (scr_game_num_scripts() <= 0)
	{
		return;
	}

	ProfileBegin(pm_update);

	ProfileBegin(pm_time_update);
	scr_time_update(L);
	ProfileEnd(pm_time_update);

	ProfileBegin(pm_game_update);
	scr_game_update(L);
	ProfileEnd(pm_game_update);

	ProfileEnd(pm_update);
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
		Con_Logf(LogSev_Error, "script", "in exec: %s", lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}

	return true;
}

bool ScriptSys_Eval(const char* script)
{
	if (luaL_dostring(L, script) != LUA_OK)
	{
		Con_Logf(LogSev_Error, "script", "in eval: %s", lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}

	return true;
}


