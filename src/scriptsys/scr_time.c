#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"
#include "scriptsys/script.h"
#include "common/console.h"
#include "common/time.h"
#include "common/profiler.h"
#include "scr_log.h"
#include "scr_time.h"

// ----------------------------------------------------------------------------

static i32 scr_func_toSec(lua_State* L);
static i32 scr_func_toMilli(lua_State* L);
static i32 scr_func_toMicro(lua_State* L);

// ----------------------------------------------------------------------------

static const luaL_Reg ms_regs[] =
{
    {.name = "toSec",.func = scr_func_toSec},
    {.name = "toMilli",.func = scr_func_toMilli},
    {.name = "toMicro",.func = scr_func_toMicro},
    {0},
};

// ----------------------------------------------------------------------------

void scr_time_init(lua_State* L)
{
    luaL_newlib(L, ms_regs);
    Script_RegisterLib(L, "Time", ScrLib_Global);
}

ProfileMark(pm_update, scr_time_update)
void scr_time_update(lua_State* L)
{
    ProfileBegin(pm_update);

    u64 frameCount = Time_FrameCount();
    u64 appStart = Time_AppStart();
    u64 frameStart = Time_FrameStart();
    u64 prevFrame = Time_PrevFrame();
    u64 now = Time_Now();
    double delta = Time_Deltaf();

    lua_getglobal(L, "Time");
    {
        lua_pushinteger(L, frameCount);
        lua_setfield(L, -2, "frameCount");

        lua_pushinteger(L, appStart);
        lua_setfield(L, -2, "appStart");

        lua_pushinteger(L, frameStart);
        lua_setfield(L, -2, "frameStart");

        lua_pushinteger(L, prevFrame);
        lua_setfield(L, -2, "prevFrame");

        lua_pushinteger(L, now);
        lua_setfield(L, -2, "now");

        lua_pushnumber(L, delta);
        lua_setfield(L, -2, "delta");
    }
    lua_pop(L, 1);

    ProfileEnd(pm_update);
}

void scr_time_shutdown(lua_State* L)
{

}

// ----------------------------------------------------------------------------

static i32 scr_func_toSec(lua_State* L)
{
    lua_Integer ticks = luaL_checkinteger(L, 1);
    lua_pushnumber(L, Time_Sec(ticks));
    return 1;
}

static i32 scr_func_toMilli(lua_State* L)
{
    lua_Integer ticks = luaL_checkinteger(L, 1);
    lua_pushnumber(L, Time_Milli(ticks));
    return 1;
}

static i32 scr_func_toMicro(lua_State* L)
{
    lua_Integer ticks = luaL_checkinteger(L, 1);
    lua_pushnumber(L, Time_Micro(ticks));
    return 1;
}
