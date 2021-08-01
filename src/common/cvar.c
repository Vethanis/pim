#include "common/cvar.h"

#include "allocator/allocator.h"
#include "common/console.h"
#include "common/fnv1a.h"
#include "common/stringutil.h"
#include "common/profiler.h"
#include "common/sort.h"
#include "common/time.h"
#include "math/scalar.h"
#include "math/float4_funcs.h"
#include "containers/sdict.h"
#include "ui/cimgui_ext.h"
#include <stdlib.h> // atof
#include <stdio.h> // sscanf
#include <string.h>

static StrDict ms_dict =
{
    .valueSize = sizeof(ConVar*),
};

void ConVar_Reg(ConVar* var)
{
    ASSERT(var);
    ASSERT(var->name);
    ASSERT(var->desc);
    ASSERT(!var->registered);

    if (!var->registered)
    {
        ConVar_SetStr(var, var->value);
        var->registered = StrDict_Add(&ms_dict, var->name, &var);
        ASSERT(var->registered);
    }
}

ConVar* ConVar_Find(const char* name)
{
    ASSERT(name);
    ConVar* value = NULL;
    StrDict_Get(&ms_dict, name, &value);
    return value;
}

const char* ConVar_Complete(const char* namePart)
{
    ASSERT(namePart);
    if (namePart[0])
    {
        const i32 partLen = StrLen(namePart);
        const u32 width = ms_dict.width;
        const char** names = ms_dict.keys;
        for (u32 i = 0; i < width; ++i)
        {
            const char* name = names[i];
            if (name && !StrCmp(namePart, partLen, name))
            {
                return name;
            }
        }
    }
    return NULL;
}

void ConVar_SetStr(ConVar* var, const char* value)
{
    ASSERT(var);
    ASSERT(value);
    if (!var->registered || (StrCmp(ARGS(var->value), value) != 0))
    {
        StrCpy(ARGS(var->value), value);
        var->modtime = Time_Now();
        switch (var->type)
        {
        default:
            ASSERT(false);
            break;
        case cvart_text:
        {
            // already in var->value
        }
        break;
        case cvart_bool:
        {
            char c = ChrLo(var->value[0]);
            var->asBool = (c == '1') || (c == 't') || (c == 'y');
        }
        break;
        case cvart_int:
        {
            var->asInt = i1_clamp(atoi(value), var->minInt, var->maxInt);
        }
        break;
        case cvart_float:
        {
            var->asFloat = f1_clamp((float)atof(value), var->minFloat, var->maxFloat);
        }
        break;
        case cvart_color:
        {
            i32 numScanned = sscanf(value, "%f %f %f %f", &var->asVector.x, &var->asVector.y, &var->asVector.z, &var->asVector.w);
            if ((numScanned == 3) || (numScanned == 4))
            {
                var->asVector = f4_saturate(var->asVector);
            }
            else
            {
                ASSERT(false);
                Con_Logf(LogSev_Error, "cvar", "Failed to parse cvar '%s' as color from '%s'", var->name, value);
                var->asVector = f4_s(0.5f);
                var->asVector.w = 1.0f;
            }
        }
        break;
        case cvart_point:
        {
            i32 numScanned = sscanf(value, "%f %f %f", &var->asVector.x, &var->asVector.y, &var->asVector.z);
            ASSERT(numScanned == 3);
            if (numScanned == 3)
            {
                var->asVector = f4_clampvs(var->asVector, var->minFloat, var->maxFloat);
            }
            else
            {
                ASSERT(false);
                Con_Logf(LogSev_Error, "cvar", "Failed to parse cvar '%s' as point from '%s'", var->name, value);
                var->asVector = f4_0;
            }
            var->asVector.w = 1.0f;
        }
        break;
        case cvart_vector:
        {
            i32 numScanned = sscanf(value, "%f %f %f", &var->asVector.x, &var->asVector.y, &var->asVector.z);
            ASSERT(numScanned == 3);
            if (numScanned == 3)
            {
                var->asVector = f4_normalize3(var->asVector);
            }
            else
            {
                ASSERT(false);
                Con_Logf(LogSev_Error, "cvar", "Failed to parse cvar '%s' as vector from '%s'", var->name, value);
                var->asVector = f4_v(0.0f, 0.0f, 1.0f, 0.0f);
            }
            var->asVector.w = 0.0f;
        }
        break;
        }
    }
}

void ConVar_SetFloat(ConVar* var, float value)
{
    ASSERT(var);
    ASSERT(var->type == cvart_float);
    ASSERT(var->registered);
    value = f1_clamp(value, var->minFloat, var->maxFloat);
    if (value != var->asFloat)
    {
        SPrintf(ARGS(var->value), "%f", value);
        var->asFloat = value;
        var->modtime = Time_Now();
    }
}

void ConVar_SetInt(ConVar* var, i32 value)
{
    ASSERT(var);
    ASSERT(var->type == cvart_int);
    ASSERT(var->registered);
    value = i1_clamp(value, var->minInt, var->maxInt);
    if (value != var->asInt)
    {
        SPrintf(ARGS(var->value), "%d", value);
        var->asInt = value;
        var->modtime = Time_Now();
    }
}

void ConVar_SetVec(ConVar* var, float4 value)
{
    ASSERT(var);
    ASSERT(var->registered);
    switch (var->type)
    {
    default:
        ASSERT(false);
        break;
    case cvart_vector:
        value = f4_normalize3(value);
        break;
    case cvart_point:
        value = f4_clampvs(value, var->minFloat, var->maxFloat);
        break;
    case cvart_color:
        value = f4_saturate(value);
        break;
    }
    if (b4_any(f4_neq(value, var->asVector)))
    {
        SPrintf(ARGS(var->value), "%f %f %f %f", value.x, value.y, value.z, value.w);
        var->asVector = value;
        var->modtime = Time_Now();
    }
}

void ConVar_SetBool(ConVar* var, bool value)
{
    ASSERT(var);
    ASSERT(var->type == cvart_bool);
    ASSERT(var->registered);
    if (value != var->asBool)
    {
        var->value[0] = value ? '1' : '0';
        var->value[1] = 0;
        var->asBool = value;
        var->modtime = Time_Now();
    }
}

bool ConVar_IsVec(const ConVar* var)
{
    switch (var->type)
    {
    default:
        return false;
    case cvart_vector:
    case cvart_point:
    case cvart_color:
        return true;
    }
}

const char* ConVar_GetStr(const ConVar* var)
{
    ASSERT(var->type == cvart_text);
    ASSERT(var->registered);
    return var->value;
}

float ConVar_GetFloat(const ConVar* var)
{
    ASSERT(var->type == cvart_float);
    ASSERT(var->registered);
    return var->asFloat;
}

i32 ConVar_GetInt(const ConVar* var)
{
    ASSERT(var->type == cvart_int);
    ASSERT(var->registered);
    return var->asInt;
}

float4 ConVar_GetVec(const ConVar* var)
{
    ASSERT(ConVar_IsVec(var));
    ASSERT(var->registered);
    return var->asVector;
}

bool ConVar_GetBool(const ConVar* var)
{
    ASSERT(var->type == cvart_bool);
    ASSERT(var->registered);
    return var->asBool;
}

void ConVar_Toggle(ConVar* var)
{
    ConVar_SetBool(var, !ConVar_GetBool(var));
}

bool ConVar_CheckDirty(const ConVar* var, u64 lastCheck)
{
    ASSERT(var->registered);
    return var->modtime > lastCheck;
}

static char ms_search[PIM_PATH];

ProfileMark(pm_gui, ConVar_Gui)
void ConVar_Gui(bool* pEnabled)
{
    ProfileBegin(pm_gui);

    const u32 ldrPicker = ImGuiColorEditFlags_Float | ImGuiColorEditFlags_InputRGB;

    if (igBegin("Config Vars", pEnabled, 0))
    {
        ConVar** pim_noalias cvars = ms_dict.values;
        const i32 length = ms_dict.count;
        const u32* pim_noalias indices = StrDict_Sort(&ms_dict, SDictStrCmp, NULL);

        igInputText("Search", ARGS(ms_search), 0, NULL, NULL);

        igSeparator();

        igExColumns(2);
        for (i32 i = 0; i < length; ++i)
        {
            u32 j = indices[i];
            ConVar* pim_noalias cvar = cvars[j];
            if (!cvar)
            {
                continue;
            }

            if (ms_search[0] && !StrIStr(cvar->name, PIM_PATH, ms_search))
            {
                continue;
            }

            switch (cvar->type)
            {
            default: ASSERT(false); break;
            case cvart_bool:
            {
                bool v = cvar->asBool;
                if (igCheckbox(cvar->name, &v))
                {
                    ConVar_SetBool(cvar, v);
                }
            }
            break;
            case cvart_text:
            {
                char buffer[sizeof(cvar->value)];
                memcpy(buffer, cvar->value, sizeof(buffer));
                if (igInputText(cvar->name, ARGS(buffer), 0, NULL, NULL))
                {
                    ConVar_SetStr(cvar, buffer);
                }
            }
            break;
            case cvart_int:
            {
                i32 v = cvar->asInt;
                if (igExSliderInt(cvar->name, &v, cvar->minInt, cvar->maxInt))
                {
                    ConVar_SetInt(cvar, v);
                }
            }
            break;
            case cvart_float:
            {
                float v = cvar->asFloat;
                if (igExSliderFloat(cvar->name, &v, cvar->minFloat, cvar->maxFloat))
                {
                    ConVar_SetFloat(cvar, v);
                }
            }
            break;
            case cvart_color:
            {
                float4 v = cvar->asVector;
                if (igColorEdit4(cvar->name, &v.x, ldrPicker))
                {
                    ConVar_SetVec(cvar, v);
                }
            }
            break;
            case cvart_point:
            {
                float4 v = cvar->asVector;
                if (igExSliderFloat4(cvar->name, &v.x, cvar->minFloat, cvar->maxFloat))
                {
                    ConVar_SetVec(cvar, v);
                }
            }
            break;
            case cvart_vector:
            {
                float4 v = cvar->asVector;
                if (igExSliderFloat3(cvar->name, &v.x, -1.0f, 1.0f))
                {
                    ConVar_SetVec(cvar, v);
                }
            }
            break;
            }
            igNextColumn();
            igText(cvar->desc); igNextColumn();
        }
        igExColumns(1);
    }
    igEnd();

    ProfileEnd(pm_gui);
}
