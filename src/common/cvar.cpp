#include "common/cvar.h"

#include "common/macro.h"
#include "common/stringutil.h"
#include "common/text.h"
#include "containers/hash_dict.h"
#include <stdlib.h>
#include <math.h>

using CVarText = Text<32>;

static HashDict<CVarText, CVar*> ms_dict;

CVar* CVar::Find(cstr name)
{
    ASSERT(name);
    CVar* ptr = 0;
    ms_dict.Get(name, ptr);
    return ptr;
}

CVar::CVar(cstr name, cstr value)
{
    StrCpy(ARGS(m_name), name);
    StrCpy(ARGS(m_value), value);
    m_asFloat = (float)atof(value);

    ms_dict.Add(name, this);
}

void CVar::Set(cstr value)
{
    StrCpy(ARGS(m_value), value);
    m_asFloat = (float)atof(value);
}

void CVar::Set(f32 value)
{
    ASSERT(value != NAN);
    char buffer[30];
    SPrintf(ARGS(buffer), "%f", value);
    Set(buffer);
}
