#pragma once

#include "common/macro.h"
#include "math/types.h"

PIM_C_BEGIN

typedef enum
{
    cvart_text = 0,
    cvart_float,
    cvart_int,
    cvart_bool,
    cvart_vector,   // unit direction
    cvart_point,    // position
    cvart_color,    // LDR color
} cvar_type;

typedef enum
{
    cvarf_dirty = 1 << 0,   // modified in the last frame
    cvarf_save = 1 << 1,    // save value to config file

} cvar_flags;

typedef struct ConVar_s
{
    cvar_type type;
    u32 flags;
    const char* name;
    char value[64];
    const char* desc;
    union
    {
        i32 minInt;
        float minFloat;
    };
    union
    {
        i32 maxInt;
        float maxFloat;
    };
    union
    {
        float4 asVector;
        float asFloat;
        i32 asInt;
        bool asBool;
    };
} ConVar_t;

// registers your cvar to the cvar system
void ConVar_Reg(ConVar_t* ptr);

// attempts to find a cvar with matching name
ConVar_t* ConVar_Find(const char* name);

// attempts to auto-complete namePart to a cvar name
const char* ConVar_Complete(const char* namePart);

// updates string and float value, sets dirty flag
void ConVar_SetStr(ConVar_t* ptr, const char* value);
void ConVar_SetFloat(ConVar_t* ptr, float value);
void ConVar_SetInt(ConVar_t* ptr, i32 value);
void ConVar_SetVec(ConVar_t* ptr, float4 value);
void ConVar_SetBool(ConVar_t* ptr, bool value);

bool ConVar_IsVec(const ConVar_t* ptr);

const char* ConVar_GetStr(const ConVar_t* ptr);
float ConVar_GetFloat(const ConVar_t* ptr);
i32 ConVar_GetInt(const ConVar_t* ptr);
float4 ConVar_GetVec(const ConVar_t* ptr);
bool ConVar_GetBool(const ConVar_t* ptr);

void ConVar_Toggle(ConVar_t* ptr);

// checks and clears dirty flag
bool ConVar_CheckDirty(ConVar_t* ptr);

// displays the cvar gui
void ConVar_Gui(bool* pEnabled);


PIM_C_END
