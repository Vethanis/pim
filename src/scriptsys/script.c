#include "scriptsys/script.h"

#include "scriptsys/scr_math.h"
#include "scriptsys/scr_log.h"
#include "scriptsys/scr_time.h"
#include "scriptsys/scr_cmd.h"
#include "scriptsys/scr_cvar.h"
#include "scriptsys/scr_game.h"

#include "allocator/allocator.h"
#include "common/console.h"
#include "common/profiler.h"
#include "common/stringutil.h"

#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

// ----------------------------------------------------------------------------

static void* scr_lua_alloc(void* ud, void* ptr, size_t osize, size_t nsize);

// ----------------------------------------------------------------------------

static lua_State* L;

// ----------------------------------------------------------------------------

void ScriptSys_Init(void)
{
    L = lua_newstate(scr_lua_alloc, NULL);
    ASSERT(L);

    luaL_openlibs(L);

    scr_math_init(L);
    scr_cmd_init(L);
    scr_log_init(L);
    scr_time_init(L);
    scr_cvar_init(L);
    scr_game_init(L);

    ScriptSys_Exec("init");
}

void ScriptSys_Shutdown(void)
{
    scr_math_shutdown(L);
    scr_game_shutdown(L);
    scr_cmd_shutdown(L);
    scr_log_shutdown(L);
    scr_time_shutdown(L);
    scr_cvar_shutdown(L);

    lua_close(L);
    L = NULL;
}

ProfileMark(pm_update, ScriptSys_Update)
void ScriptSys_Update(void)
{
    ProfileBegin(pm_update);

    scr_time_update(L);
    scr_game_update(L);

    ProfileEnd(pm_update);
}

void Script_RegisterLib(lua_State* L, const char* name, ScrLib_Reg regType)
{
    ASSERT(name);
    switch (regType)
    {
    default:
        ASSERT(false);
        break;
    case ScrLib_Global:
    {
        lua_setglobal(L, name);
    }
    break;
    case ScrLib_Import:
    {
        lua_getglobal(L, "package");
        lua_getfield(L, -1, "loaded");
        lua_pushvalue(L, 1);
        lua_setfield(L, -2, name);
        lua_pop(L, 3);
    }
    break;
    }
}

ProfileMark(pm_exec, ScriptSys_Exec)
bool ScriptSys_Exec(const char* filename)
{
    ProfileBegin(pm_exec);

    ASSERT(filename);

    bool success = true;
    char path[PIM_PATH] = { 0 };
    StrCpy(ARGS(path), SCRIPT_DIR);
    i32 len = StrCat(ARGS(path), filename);
    if (!IEndsWith(ARGS(path), ".lua"))
    {
        len = StrCat(ARGS(path), ".lua");
    }

    if (len > PIM_PATH)
    {
        Con_Logf(LogSev_Error, "scr", "path is too long (> %d): '%s' ", PIM_PATH, path);
        success = false;
        goto cleanup;
    }

    Con_Logf(LogSev_Verbose, "scr", "executing script from '%s'", path);

    if (luaL_dofile(L, path) != LUA_OK)
    {
        Con_Logf(LogSev_Error, "scr", "in exec: '%s'", lua_tostring(L, -1));
        lua_pop(L, 1);
        success = false;
        goto cleanup;
    }

cleanup:
    ProfileEnd(pm_exec);
    return success;
}

ProfileMark(pm_eval, ScriptSys_Eval)
bool ScriptSys_Eval(const char* expression)
{
    ProfileBegin(pm_eval);

    ASSERT(expression);
    bool success = true;

    if (luaL_dostring(L, expression) != LUA_OK)
    {
        Con_Logf(LogSev_Error, "scr", "in eval: %s", lua_tostring(L, -1));
        lua_pop(L, 1);
        success = false;
        goto cleanup;
    }

cleanup:
    ProfileEnd(pm_eval);
    return success;
}

// ----------------------------------------------------------------------------

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

