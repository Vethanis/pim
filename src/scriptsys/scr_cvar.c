#include "scriptsys/scr_cvar.h"

#include "scriptsys/script.h"
#include "scriptsys/scr_cmd.h"

#include "common/console.h"
#include "common/stringutil.h"
#include "common/cvar.h"
#include "math/types.h"
#include "math/float4_funcs.h"

#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>
#include <string.h>

#define SCR_F4_USERDATA "Pim.float4"

// ----------------------------------------------------------------------------

static void VEC_CALL scr_push_f4(lua_State* L, float4 vec);
static float scr_checknumberfield(lua_State* L, i32 pos, const char* field);
static float4 scr_check_f4(lua_State* L, i32 pos);
static i32 scr_func_get(lua_State* L);
static i32 scr_func_set(lua_State* L);
static i32 scr_f4_new(lua_State* L);
static i32 scr_f4_tostring(lua_State* L);
static void scr_f4_formatstring(lua_State* L, char buf[64], i32 n);
static i32 scr_f4_index(lua_State* L);
static i32 scr_f4_concat(lua_State* L);

// ----------------------------------------------------------------------------

static const luaL_Reg ms_regs[] =
{
    {.name = "get",.func = scr_func_get },
    {.name = "set",.func = scr_func_set },
    { 0 }
};


// normal functions
static const luaL_Reg float4_regs_f[] =
{
    {.name = "new",.func = scr_f4_new },
    { 0 }
};

// metatable functions
static const luaL_Reg float4_regs_m[] =
{
    {.name = "__index",.func = scr_f4_index },
    {.name = "__tostring",.func = scr_f4_tostring },
    {.name = "__concat",.func = scr_f4_concat },
    { 0 }
};

static const char* float4_fields[] = { "x", "y", "z", "w" };

// ----------------------------------------------------------------------------

void scr_cvar_init(lua_State* L)
{
    luaL_newlib(L, ms_regs);
    Script_RegisterLib(L, "cvar", ScrLib_Import);

    luaL_newmetatable(L, SCR_F4_USERDATA);
    lua_pushvalue(L, -1); // duplicate metatable
    lua_setfield(L, -2, "__index"); // mt.__index = mt
    luaL_setfuncs(L, float4_regs_m, 0);
    luaL_newlib(L, float4_regs_f);
    Script_RegisterLib(L, "float4", ScrLib_Global);
    lua_pop(L, 1); // pop metatable
}

void scr_cvar_shutdown(lua_State* L)
{

}

// ----------------------------------------------------------------------------

static void VEC_CALL scr_push_f4(lua_State* L, float4 vec)
{
    float4* luaVec = (float4*)lua_newuserdatauv(L, sizeof(float4), 0);
    luaL_getmetatable(L, SCR_F4_USERDATA);
    lua_setmetatable(L, -2);

    *luaVec = vec;
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

    // Can either be separated args, userdata, or a table with the fields
    i32 argc = lua_gettop(L);
    if (lua_isuserdata(L, pos))
    {
        float4* luaVec = luaL_checkudata(L, pos, SCR_F4_USERDATA);
        val = *luaVec;
    }
    else if (lua_istable(L, pos))
    {
        val.x = scr_checknumberfield(L, pos, "x");
        val.y = scr_checknumberfield(L, pos, "y");
        val.z = scr_checknumberfield(L, pos, "z");
        val.w = scr_checknumberfield(L, pos, "w");
    }
    else
    {
        val.x = (float)luaL_checknumber(L, pos);
        if (lua_isnumber(L, pos + 1))
            val.y = (float)lua_tonumber(L, pos + 1);
        else
            return val;

        if (lua_isnumber(L, pos + 2))
            val.z = (float)lua_tonumber(L, pos + 2);
        else
            return val;

        if (lua_isnumber(L, pos + 3))
            val.w = (float)lua_tonumber(L, pos + 3);
        else
            return val;
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

static i32 scr_f4_new(lua_State* L)
{
    float4 luaVec = scr_check_f4(L, 1);
    scr_push_f4(L, luaVec);
    return 1;
}

static i32 scr_f4_tostring(lua_State* L)
{
    char buf[64];
    scr_f4_formatstring(L, buf, 1);
    lua_pushstring(L, buf);
    return 1;
}

static void scr_f4_formatstring(lua_State* L, char buf[64], i32 n)
{
    float4* luaVec = (float4*)luaL_checkudata(L, n, SCR_F4_USERDATA);
    SPrintf(buf, 64, "float4(%f,%f,%f,%f)", luaVec->x, luaVec->y, luaVec->z, luaVec->w);
}

static i32 scr_f4_concat(lua_State* L)
{
    char vecStr[64];
    char* a;
    char* b;
    if (lua_isuserdata(L, 1))
    {
        scr_f4_formatstring(L, vecStr, 1);
        a = vecStr;
        b = lua_tostring(L, 2);
    }
    else
    {
        a = lua_tostring(L, 1);
        scr_f4_formatstring(L, vecStr, 2);
        b = vecStr;
    }

    lua_pushfstring(L, "%s%s", a, b);
    return 1;
}

static i32 scr_f4_index(lua_State* L)
{
    float4* luaVec = (float4*)luaL_checkudata(L, 1, SCR_F4_USERDATA);
    i32 elem = luaL_checkoption(L, 2, NULL, float4_fields);
    luaL_argcheck(L, luaVec != NULL, 1, "'float4' expected");
    luaL_argcheck(L, elem >= 1 && elem <= 4, 2, "expected one of: x,y,z,w");

    lua_pushnumber(L, f4_get(*luaVec, elem));
    return 1;
}
