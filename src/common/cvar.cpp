#include "common/cvar.h"

#include "common/macro.h"
#include "common/stringutil.h"
#include <stdlib.h>

CVar::CVar(const char* name, const char* value)
{
    ASSERT(name);
    ASSERT(value);
    StrCpy(ARGS(m_name), name);
    StrCpy(ARGS(m_value), value);
    m_asFloat = (float)atof(value);
}

void CVar::Set(f32 value)
{
    char buffer[30];
    SPrintf(ARGS(buffer), "%f", value);
    Set(buffer);
}
