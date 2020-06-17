#pragma once

#include "common/macro.h"

PIM_C_BEGIN

u32 sset_hash(const char* key);
i32 sset_find(const char** keys, const u32* hashes, i32 width, const char* key);

PIM_C_END
