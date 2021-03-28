#include "common/cvar.h"

#include "allocator/allocator.h"
#include "common/fnv1a.h"
#include "common/stringutil.h"
#include "common/profiler.h"
#include "common/sort.h"
#include "math/scalar.h"
#include "math/float4_funcs.h"
#include "containers/sdict.h"
#include "ui/cimgui_ext.h"
#include <stdlib.h> // atof
#include <stdio.h> // sscanf

static StrDict ms_dict =
{
    .valueSize = sizeof(ConVar_t*),
};

void ConVar_Reg(ConVar_t* ptr)
{
    ASSERT(ptr);
    ASSERT(ptr->name);
    ASSERT(ptr->desc);

    ConVar_SetStr(ptr, ptr->value);
    ptr->flags &= ~cvarf_dirty;

    bool added = StrDict_Add(&ms_dict, ptr->name, &ptr);
    ASSERT(added);
}

ConVar_t* ConVar_Find(const char* name)
{
    ConVar_t* value = NULL;
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

void ConVar_SetStr(ConVar_t* ptr, const char* value)
{
    ASSERT(ptr);
    ASSERT(value);
    ptr->flags |= cvarf_dirty;
    if (ptr->value != value)
    {
        StrCpy(ARGS(ptr->value), value);
    }
    switch (ptr->type)
    {
    default:
        ASSERT(false);
        break;
    case cvart_text:
    {
        // already in ptr->value
    }
    break;
    case cvart_bool:
    {
        ptr->asBool = ptr->value[0] == '1';
    }
    break;
    case cvart_int:
    {
        ptr->asInt = i1_clamp(atoi(value), ptr->minInt, ptr->maxInt);
    }
    break;
    case cvart_float:
    {
        ptr->asFloat = f1_clamp((float)atof(value), ptr->minFloat, ptr->maxFloat);
    }
    break;
    case cvart_color:
    {
        i32 numScanned = sscanf(value, "%f %f %f %f", &ptr->asVector.x, &ptr->asVector.y, &ptr->asVector.z, &ptr->asVector.w);
        if (numScanned == 3 || numScanned == 4)
        {
            ptr->asVector = f4_saturate(ptr->asVector);
        }
        else
        {
            ASSERT(false);
            ptr->asVector = f4_s(0.5f);
            ptr->asVector.w = 1.0f;
        }
    }
    break;
    case cvart_point:
    {
        i32 numScanned = sscanf(value, "%f %f %f", &ptr->asVector.x, &ptr->asVector.y, &ptr->asVector.z);
        ASSERT(numScanned == 3);
        if (numScanned == 3)
        {
            ptr->asVector = f4_clampvs(ptr->asVector, ptr->minFloat, ptr->maxFloat);
        }
        else
        {
            ptr->asVector = f4_0;
        }
        ptr->asVector.w = 1.0f;
    }
    break;
    case cvart_vector:
    {
        i32 numScanned = sscanf(value, "%f %f %f", &ptr->asVector.x, &ptr->asVector.y, &ptr->asVector.z);
        ASSERT(numScanned == 3);
        if (numScanned == 3)
        {
            ptr->asVector = f4_normalize3(ptr->asVector);
        }
        else
        {
            ptr->asVector = f4_v(0.0f, 0.0f, 1.0f, 0.0f);
        }
        ptr->asVector.w = 0.0f;
    }
    break;
    }
}

void ConVar_SetFloat(ConVar_t* ptr, float value)
{
    ASSERT(ptr);
    ASSERT(ptr->type == cvart_float);
    value = f1_clamp(value, ptr->minFloat, ptr->maxFloat);
    ptr->flags |= cvarf_dirty;
    SPrintf(ARGS(ptr->value), "%f", value);
    ptr->asFloat = value;
}

void ConVar_SetInt(ConVar_t* ptr, i32 value)
{
    ASSERT(ptr);
    ASSERT(ptr->type == cvart_int);
    value = i1_clamp(value, ptr->minInt, ptr->maxInt);
    ptr->flags |= cvarf_dirty;
    SPrintf(ARGS(ptr->value), "%d", value);
    ptr->asInt = value;
}

void ConVar_SetVec(ConVar_t* ptr, float4 value)
{
    ASSERT(ptr);
    switch (ptr->type)
    {
    default:
        ASSERT(false);
        break;
    case cvart_vector:
        value = f4_normalize3(value);
        break;
    case cvart_point:
        value = f4_clampvs(value, ptr->minFloat, ptr->maxFloat);
        break;
    case cvart_color:
        value = f4_saturate(value);
        break;
    }
    ptr->flags |= cvarf_dirty;
    SPrintf(ARGS(ptr->value), "%f %f %f %f", value.x, value.y, value.z, value.w);
    ptr->asVector = value;
}

void ConVar_SetBool(ConVar_t* ptr, bool value)
{
    ASSERT(ptr);
    ASSERT(ptr->type == cvart_bool);
    ptr->flags |= cvarf_dirty;
    ptr->value[0] = value ? '1' : '0';
    ptr->value[1] = 0;
    ptr->asBool = value;
}

bool ConVar_IsVec(const ConVar_t* ptr)
{
    switch (ptr->type)
    {
    default:
        return false;
    case cvart_vector:
    case cvart_point:
    case cvart_color:
        return true;
    }
}

const char* ConVar_GetStr(const ConVar_t* ptr)
{
    ASSERT(ptr->type == cvart_text);
    return ptr->value;
}

float ConVar_GetFloat(const ConVar_t* ptr)
{
    ASSERT(ptr->type == cvart_float);
    return ptr->asFloat;
}

i32 ConVar_GetInt(const ConVar_t* ptr)
{
    ASSERT(ptr->type == cvart_int);
    return ptr->asInt;
}

float4 ConVar_GetVec(const ConVar_t* ptr)
{
    ASSERT(ConVar_IsVec(ptr));
    return ptr->asVector;
}

bool ConVar_GetBool(const ConVar_t* ptr)
{
    ASSERT(ptr->type == cvart_bool);
    return ptr->asBool;
}

void ConVar_Toggle(ConVar_t* ptr)
{
    ConVar_SetBool(ptr, !ConVar_GetBool(ptr));
}

bool ConVar_CheckDirty(ConVar_t* ptr)
{
    ASSERT(ptr);
    bool dirty = (ptr->flags & cvarf_dirty) != 0;
    ptr->flags &= ~cvarf_dirty;
    return dirty;
}

static char ms_search[PIM_PATH];

ProfileMark(pm_gui, ConVar_Gui)
void ConVar_Gui(bool* pEnabled)
{
    ProfileBegin(pm_gui);

    const u32 ldrPicker = ImGuiColorEditFlags_Float | ImGuiColorEditFlags_InputRGB;

    if (igBegin("Config Vars", pEnabled, 0))
    {
        ConVar_t** cvars = ms_dict.values;
        const i32 length = ms_dict.count;
        const u32* indices = StrDict_Sort(&ms_dict, SDictStrCmp, NULL);

        igInputText("Search", ARGS(ms_search), 0, NULL, NULL);

        igSeparator();

        igExColumns(2);
        for (i32 i = 0; i < length; ++i)
        {
            u32 j = indices[i];
            ConVar_t* cvar = cvars[j];
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
                if (igCheckbox(cvar->name, &cvar->asBool))
                {
                    ConVar_SetBool(cvar, cvar->asBool);
                }
            }
            break;
            case cvart_text:
            {
                if (igInputText(cvar->name, ARGS(cvar->value), 0, NULL, NULL))
                {
                    cvar->flags |= cvarf_dirty;
                }
            }
            break;
            case cvart_int:
            {
                if (igExSliderInt(cvar->name, &cvar->asInt, cvar->minInt, cvar->maxInt))
                {
                    ConVar_SetInt(cvar, cvar->asInt);
                }
            }
            break;
            case cvart_float:
            {
                if (igExSliderFloat(cvar->name, &cvar->asFloat, cvar->minFloat, cvar->maxFloat))
                {
                    ConVar_SetFloat(cvar, cvar->asFloat);
                }
            }
            break;
            case cvart_color:
            {
                if (igColorEdit4(cvar->name, &cvar->asVector.x, ldrPicker))
                {
                    ConVar_SetVec(cvar, cvar->asVector);
                }
            }
            break;
            case cvart_point:
            {
                if (igExSliderFloat4(cvar->name, &cvar->asVector.x, cvar->minFloat, cvar->maxFloat))
                {
                    ConVar_SetVec(cvar, cvar->asVector);
                }
            }
            break;
            case cvart_vector:
            {
                if (igExSliderFloat3(cvar->name, &cvar->asVector.x, -1.0f, 1.0f))
                {
                    ConVar_SetVec(cvar, cvar->asVector);
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
