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
    cvart_color,    // color
} cvar_type;

typedef enum
{
    cvarf_save = 1 << 0,        // save value to config file
    cvarf_logarithmic = 1 << 1, // logarithmic slider / picker
    cvarf_hdr = 1 << 2,         // high dynamic range
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
    u64 modtime;
    bool registered;
} ConVar;

// registers your cvar to the cvar system
void ConVar_Reg(ConVar* var);

// attempts to find a cvar with matching name
ConVar* ConVar_Find(const char* name);

// attempts to auto-complete namePart to a cvar name
const char* ConVar_Complete(const char* namePart);

// updates string and float value, sets dirty flag
void ConVar_SetStr(ConVar* ptr, const char* value);
void ConVar_SetFloat(ConVar* ptr, float value);
void ConVar_SetInt(ConVar* ptr, i32 value);
void ConVar_SetVec(ConVar* ptr, float4 value);
void ConVar_SetBool(ConVar* ptr, bool value);

bool ConVar_IsVec(const ConVar* ptr);

const char* ConVar_GetStr(const ConVar* ptr);
float ConVar_GetFloat(const ConVar* ptr);
i32 ConVar_GetInt(const ConVar* ptr);
float4 ConVar_GetVec(const ConVar* ptr);
bool ConVar_GetBool(const ConVar* ptr);

void ConVar_Toggle(ConVar* ptr);

bool ConVar_CheckDirty(const ConVar* ptr, u64 lastCheck);

// displays the cvar gui
void ConVar_Gui(bool* pEnabled);


PIM_C_END
