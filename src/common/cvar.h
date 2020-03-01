#pragma once

#include "common/int_types.h"

struct CVar
{
public:
    CVar(const char* name, const char* value);
    void Set(const char* value) { *this = CVar(m_name, value); }
    void Set(f32 value);
    const char* GetName() const { return m_name; }
    const char* GetValue() const { return m_value; }
    f32 AsFloat() const { return m_asFloat; }
private:
    char m_name[30];
    char m_value[30];
    f32 m_asFloat;
};
