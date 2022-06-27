#include <lua/lua.h>
#include <lua/lauxlib.h>

#include <scriptsys/scr_util.h>

// ----------------------------------------------------------------------------

float scr_checknumberfield(lua_State* L, i32 pos, const char* field)
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
