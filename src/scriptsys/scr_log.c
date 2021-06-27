#include "scriptsys/scr_log.h"

#include "scriptsys/script.h"

#include "common/console.h"

#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

// ----------------------------------------------------------------------------

static i32 scr_log_sev(lua_State* L, LogSev severity);

static i32 scr_func_info(lua_State* L);
static i32 scr_func_error(lua_State* L);
static i32 scr_func_warning(lua_State* L);
static i32 scr_func_verbose(lua_State* L);

// ----------------------------------------------------------------------------

static const luaL_Reg ms_regs[] =
{
    {.name = "info",.func = scr_func_info},
    {.name = "error",.func = scr_func_error},
    {.name = "warning",.func = scr_func_warning},
    {.name = "verbose",.func = scr_func_verbose},
    {0},
};

// ----------------------------------------------------------------------------

void scr_log_init(lua_State* L)
{
    luaL_newlib(L, ms_regs);
    Script_RegisterLib(L, "Log", ScrLib_Global);
}

void scr_log_shutdown(lua_State* L)
{

}

// ----------------------------------------------------------------------------

static i32 scr_log_sev(lua_State* L, LogSev severity)
{
    lua_concat(L, lua_gettop(L));
    const char* message = luaL_tolstring(L, 1, NULL);
    lua_pop(L, 1);

    Con_Logf(severity, "scr", message);
    return 0;
}

static i32 scr_func_info(lua_State* L)
{
    return scr_log_sev(L, LogSev_Info);
}

static i32 scr_func_error(lua_State* L)
{
    return scr_log_sev(L, LogSev_Error);
}

static i32 scr_func_warning(lua_State* L)
{
    return scr_log_sev(L, LogSev_Warning);
}

static i32 scr_func_verbose(lua_State* L)
{
    return scr_log_sev(L, LogSev_Verbose);
}
