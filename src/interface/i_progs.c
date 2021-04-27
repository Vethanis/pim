#include "interface/i_progs.h"

#if QUAKE_IMPL

#include "interface/i_globals.h"

const i32 g_type_size[ev_COUNT] =
{
    sizeof(i32) / 4,        // ev_void
    sizeof(string_t) / 4,   // ev_string
    sizeof(float) / 4,      // ev_float
    sizeof(vec3_t) / 4,     // ev_vector
    sizeof(i32) / 4,        // ev_entity
    sizeof(i32) / 4,        // ev_field
    sizeof(i32) / 4,        // ev_function
    sizeof(isize) / 4,      // ev_pointer
};

#endif // QUAKE_IMPL
