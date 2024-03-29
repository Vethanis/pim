#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct lua_State lua_State;

void scr_game_init(lua_State* L);
void scr_game_shutdown(lua_State* L);
void scr_game_update(lua_State* L);
i32 scr_game_num_scripts(void);

PIM_C_END
