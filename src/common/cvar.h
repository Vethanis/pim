#pragma once

#include "common/int_types.h"

struct CVar
{
public:
    CVar(cstr name, cstr value);
    void Set(cstr value);
    void Set(f32 value);
    cstr GetName() const { return m_name; }
    cstr GetValue() const { return m_value; }
    f32 AsFloat() const { return m_asFloat; }
    i32 AsInt() const { return (i32)m_asFloat; }
    bool AsBool() const { return m_asFloat > 0.0f; }

    static CVar* Find(cstr name);
private:
    char m_name[30];
    char m_value[30];
    f32 m_asFloat;
};
