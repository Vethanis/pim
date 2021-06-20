#include "script/script.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"

void ScriptSys_Init()
{
	ms_luastate = luaL_newstate();
	luaL_openlibs(ms_luastate);

	luaL_dostring(ms_luastate, "print(\"Hello World!\", \"2 * 2 is \", 2*2)");
}
