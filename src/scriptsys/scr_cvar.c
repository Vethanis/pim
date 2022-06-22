#include "scriptsys/scr_cvar.h"

#include "scriptsys/script.h"
#include "scriptsys/scr_cmd.h"

#include "common/console.h"
#include "common/cvar.h"
#include "math/types.h"

#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

// ----------------------------------------------------------------------------

static void VEC_CALL scr_push_f4(lua_State* L, float4 vec);
static float scr_checknumberfield(lua_State* L, i32 pos, const char* field);
static float4 scr_check_f4(lua_State* L, i32 pos);
static i32 scr_func_get(lua_State* L);
static i32 scr_func_set(lua_State* L);

// ----------------------------------------------------------------------------

static const luaL_Reg ms_regs[] =
{
    {.name = "get",.func = scr_func_get },
    {.name = "set",.func = scr_func_set },
    { 0 }
};

// ----------------------------------------------------------------------------

void scr_cvar_init(lua_State* L)
{
    luaL_newlib(L, ms_regs);
    Script_RegisterLib(L, "cvar", ScrLib_Import);
}

void scr_cvar_shutdown(lua_State* L)
{

}

// ----------------------------------------------------------------------------

static void VEC_CALL scr_push_f4(lua_State* L, float4 vec)
{
    lua_createtable(L, 4, 0);
    lua_pushnumber(L, vec.x);
    lua_setfield(L, -2, "x");
    lua_pushnumber(L, vec.y);
    lua_setfield(L, -2, "y");
    lua_pushnumber(L, vec.z);
    lua_setfield(L, -2, "z");
    lua_pushnumber(L, vec.w);
    lua_setfield(L, -2, "w");
}

static float scr_checknumberfield(lua_State* L, i32 pos, const char* field)
{
    lua_getfield(L, pos, field);
    i32 isNum = 0;
    float num = (float)lua_tonumberx(L, pos + 1, &isNum);
    if (!isNum)
    {
        luaL_error(L, "required field %s was not a number", field);
    }

    lua_pop(L, 1);
    return num;
}

static float4 scr_check_f4(lua_State* L, i32 pos)
{
    float4 val = { 0 };

    // Can either be separated args, or a table with the fields
    if (lua_istable(L, pos))
    {
        val.x = scr_checknumberfield(L, pos, "x");
        val.y = scr_checknumberfield(L, pos, "y");
        val.z = scr_checknumberfield(L, pos, "z");
        val.w = scr_checknumberfield(L, pos, "w");
    }
    else
    {
        val.x = (float)luaL_checknumber(L, pos);
        val.y = (float)luaL_checknumber(L, pos + 1);
        val.z = (float)luaL_checknumber(L, pos + 2);
        val.w = (float)luaL_checknumber(L, pos + 3);
    }

    return val;
}

static i32 scr_func_get(lua_State* L)
{
    const char* name = luaL_checkstring(L, 1);

    ConVar* var = ConVar_Find(name);
    if (!var)
    {
        Con_Logf(LogSev_Warning, "scr", "no cvar '%s' exists", name);
        return 0;
    }

    switch (var->type)
    {
    default:
        ASSERT(false);
        break;
    case cvart_bool:
        lua_pushboolean(L, ConVar_GetBool(var));
        break;
    case cvart_float:
        lua_pushnumber(L, ConVar_GetFloat(var));
        break;
    case cvart_int:
        lua_pushinteger(L, ConVar_GetInt(var));
        break;
    case cvart_text:
        lua_pushstring(L, ConVar_GetStr(var));
        break;
    case cvart_color:
    case cvart_vector:
    case cvart_point:
        scr_push_f4(L, ConVar_GetVec(var));
        break;
    }

    return 1;
}

static i32 scr_func_set(lua_State* L)
{
    const char* name = luaL_checkstring(L, 1);

    ConVar* var = ConVar_Find(name);
    if (!var)
    {
        Con_Logf(LogSev_Warning, "scr", "no cvar '%s' exists", name);
        return 0;
    }

    switch (var->type)
    {
    default:
        ASSERT(false);
        break;
    case cvart_bool:
    {
        bool val = lua_toboolean(L, 2);
        ConVar_SetBool(var, val);
    }
    break;
    case cvart_float:
    {
        float val = (float)luaL_checknumber(L, 2);
        ConVar_SetFloat(var, val);
    }
    break;
    case cvart_int:
    {
        i32 val = (i32)luaL_checkinteger(L, 2);
        ConVar_SetInt(var, val);
    }
    break;
    case cvart_text:
    {
        const char* val = luaL_checkstring(L, 2);
        ConVar_SetStr(var, val);
    }
    break;
    case cvart_color:
    case cvart_vector:
    case cvart_point:
    {
        float4 val = scr_check_f4(L, 2);
        ConVar_SetVec(var, val);
    }
    break;
    }

    Con_Logf(LogSev_Verbose, "scr", "'%s' = '%s'", var->name, var->value);

    return 0;
}
