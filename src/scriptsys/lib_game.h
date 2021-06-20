#include "script.h"
#include "common/macro.h"

PIM_C_BEGIN

void lib_game_init(lua_State* L);
void lib_game_shutdown(lua_State* L);
void lib_game_update(lua_State* L);
i32 lib_game_num_scripts(void);

PIM_C_END
