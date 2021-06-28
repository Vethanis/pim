#include "scriptsys/scr_game.h"

#include "scriptsys/script.h"
#include "scriptsys/scr_log.h"

#include "allocator/allocator.h"
#include "common/console.h"
#include "common/profiler.h"
#include "containers/hash_set.h"

#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

// ----------------------------------------------------------------------------

typedef i32 RefId;

static i32 scr_func_start_update(lua_State* L);
static i32 scr_func_stop_update(lua_State* L);
static void scr_remove_update_handler(lua_State* L, RefId refId);

// ----------------------------------------------------------------------------

static const luaL_Reg ms_regs[] =
{
    {.name = "start_update",.func = scr_func_start_update },
    {.name = "stop_update",.func = scr_func_stop_update },
    {0},
};

static HashSet ms_update_handlers =
{
    .keySize = sizeof(RefId),
};

// ----------------------------------------------------------------------------

void scr_game_init(lua_State* L)
{
    luaL_newlib(L, ms_regs);
    Script_RegisterLib(L, "Game", ScrLib_Global);
}

void scr_game_shutdown(lua_State* L)
{
    const RefId* refIds = ms_update_handlers.keys;
    const i32 width = ms_update_handlers.width;
    for (i32 i = 0; i < width; i++)
    {
        i32 refId = refIds[i];
        if (!refId)
        {
            continue;
        }

        scr_remove_update_handler(L, refId);
    }

    HashSet_Del(&ms_update_handlers);
}

ProfileMark(pm_update, scr_game_update);
void scr_game_update(lua_State* L)
{
    ProfileBegin(pm_update);

    if (ms_update_handlers.count > 0)
    {
        const RefId* refIds = ms_update_handlers.keys;
        const i32 width = ms_update_handlers.width;
        for (i32 i = 0; i < width; i++)
        {
            i32 refId = refIds[i];
            if (!refId)
            {
                continue;
            }

            lua_rawgeti(L, LUA_REGISTRYINDEX, refId);
            lua_getfield(L, 1, "update");
            lua_pushvalue(L, 1); // self arg
            if (lua_pcall(L, 1, 0, 0) != LUA_OK)
            {
                Con_Logf(LogSev_Error, "scr", "in update: %s", lua_tostring(L, -1));
                lua_pop(L, 1); // error, table
                scr_remove_update_handler(L, refId);
            }

            lua_pop(L, 1); // table
        }
    }

    ProfileEnd(pm_update);
}

i32 scr_game_num_scripts(void)
{
    return ms_update_handlers.count;
}

// ----------------------------------------------------------------------------

static i32 scr_func_start_update(lua_State* L)
{
    lua_pushvalue(L, 1);
    RefId refId = luaL_ref(L, LUA_REGISTRYINDEX);
    if (refId == LUA_REFNIL)
    {
        ASSERT(false);
        return 0;
    }

    Con_Logf(LogSev_Verbose, "scr", "starting script activity #%d", refId);

    if (lua_getfield(L, 1, "start") != LUA_TNIL)
    {
        lua_pushvalue(L, 1); // self arg
        if (lua_pcall(L, 1, 0, 0) != LUA_OK)
        {
            Con_Logf(LogSev_Error, "scr", "in start_update: %s", lua_tostring(L, -1));
            lua_pop(L, 1); // error
            return 0;
        }
    }
    else
    {
        lua_pop(L, 1); // nil field
    }

    lua_pushinteger(L, refId);
    lua_setfield(L, 1, "__update_ref"); // set table.__update_ref for later unregistering
    HashSet_Add(&ms_update_handlers, &refId, sizeof(refId));

    return 0;
}

static i32 scr_func_stop_update(lua_State* L)
{
    lua_getfield(L, 1, "__update_ref");
    RefId refId = (RefId)lua_tointeger(L, 2);

    scr_remove_update_handler(L, refId);
    return 0;
}

static void scr_remove_update_handler(lua_State* L, RefId refId)
{
    Con_Logf(LogSev_Verbose, "scr", "stopping script activity #%d", refId);

    lua_rawgeti(L, LUA_REGISTRYINDEX, refId);
    if (lua_getfield(L, 1, "stop") != LUA_TNIL)
    {
        lua_pushvalue(L, 1); // self arg
        if (lua_pcall(L, 1, 0, 0) != LUA_OK)
        {
            Con_Logf(LogSev_Error, "scr", "in remove_update: %s", lua_tostring(L, -1));
            lua_pop(L, 1); // error
        }
    }
    else
    {
        lua_pop(L, 1); // nil field
    }
    lua_pop(L, 1); // table

    HashSet_Rm(&ms_update_handlers, &refId, sizeof(refId));
    luaL_unref(L, LUA_REGISTRYINDEX, refId);
}
