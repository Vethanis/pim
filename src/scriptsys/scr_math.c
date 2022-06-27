#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>
#include <string.h>

#include "common/stringutil.h"
#include "scriptsys/script.h"
#include "scriptsys/scr_util.h"
#include "scriptsys/scr_math.h"
#include "math/float4_funcs.h"

#define SCR_F4_USERDATA "Pim.float4"

// ----------------------------------------------------------------------------

static i32 scr_f4_new(lua_State* L);
static i32 scr_f4_tostring(lua_State* L);
static void scr_f4_formatstring(lua_State* L, char buf[64], i32 n);
static i32 scr_f4_index(lua_State* L);
static i32 scr_f4_concat(lua_State* L);

// ----------------------------------------------------------------------------

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

void scr_math_init(lua_State* L)
{
    luaL_newmetatable(L, SCR_F4_USERDATA);
    lua_pushvalue(L, -1); // duplicate metatable
    lua_setfield(L, -2, "__index"); // mt.__index = mt
    luaL_setfuncs(L, float4_regs_m, 0);
    luaL_newlib(L, float4_regs_f);
    Script_RegisterLib(L, "float4", ScrLib_Global);
    lua_pop(L, 1); // pop metatable
}

void scr_math_shutdown(lua_State* L)
{

}

void VEC_CALL scr_push_f4(lua_State* L, float4 vec)
{
    float4* luaVec = (float4*)lua_newuserdatauv(L, sizeof(float4), 0);
    luaL_getmetatable(L, SCR_F4_USERDATA);
    lua_setmetatable(L, -2);

    *luaVec = vec;
}

float4 scr_check_f4(lua_State* L, i32 pos)
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
