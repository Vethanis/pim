#pragma once

#include "common/int_types.h"

using index_t = i32;
using version_t = i16;

struct Entity
{
    index_t index;
    version_t version;
};
