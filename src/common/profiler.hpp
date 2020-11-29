#pragma once

#include "common/profiler.h"

#if PIM_PROFILE

struct _ProfileScope
{
    profmark_t& m_mark;
    _ProfileScope(profmark_t& mark) : m_mark(mark)
    {
        _ProfileBegin(&m_mark);
    }
    ~_ProfileScope()
    {
        _ProfileEnd(&m_mark);
    }
};

#define ProfileScope(mark) _ProfileScope CAT_TOK(mark, _scope)(mark)

#else

#define ProfileScope(mark) 

#endif // PIM_PROFILE
