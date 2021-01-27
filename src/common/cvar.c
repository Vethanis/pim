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

static sdict_t ms_dict;

static void EnsureInit(void)
{
    if (!ms_dict.valueSize)
    {
        sdict_new(&ms_dict, sizeof(cvar_t*), EAlloc_Perm);
    }
}

void cvar_reg(cvar_t* ptr)
{
    EnsureInit();

    ASSERT(ptr);
    ASSERT(ptr->name);
    ASSERT(ptr->desc);

    cvar_set_str(ptr, ptr->value);
    ptr->flags &= ~cvarf_dirty;

    bool added = sdict_add(&ms_dict, ptr->name, &ptr);
    ASSERT(added);
}

cvar_t* cvar_find(const char* name)
{
    EnsureInit();

    cvar_t* value = NULL;
    sdict_get(&ms_dict, name, &value);
    return value;
}

const char* cvar_complete(const char* namePart)
{
    EnsureInit();

    ASSERT(namePart);
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
    return NULL;
}

void cvar_set_str(cvar_t* ptr, const char* value)
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

void cvar_set_float(cvar_t* ptr, float value)
{
    ASSERT(ptr);
    ASSERT(ptr->type == cvart_float);
    value = f1_clamp(value, ptr->minFloat, ptr->maxFloat);
    ptr->flags |= cvarf_dirty;
    SPrintf(ARGS(ptr->value), "%f", value);
    ptr->asFloat = value;
}

void cvar_set_int(cvar_t* ptr, i32 value)
{
    ASSERT(ptr);
    ASSERT(ptr->type == cvart_int);
    value = i1_clamp(value, ptr->minInt, ptr->maxInt);
    ptr->flags |= cvarf_dirty;
    SPrintf(ARGS(ptr->value), "%d", value);
    ptr->asInt = value;
}

void cvar_set_vec(cvar_t* ptr, float4 value)
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

void cvar_set_bool(cvar_t* ptr, bool value)
{
    ASSERT(ptr);
    ASSERT(ptr->type == cvart_bool);
    ptr->flags |= cvarf_dirty;
    ptr->value[0] = value ? '1' : '0';
    ptr->value[1] = 0;
    ptr->asBool = value;
}

bool cvar_check_dirty(cvar_t* ptr)
{
    ASSERT(ptr);
    bool dirty = (ptr->flags & cvarf_dirty) != 0;
    ptr->flags &= ~cvarf_dirty;
    return dirty;
}

static char ms_search[PIM_PATH];

ProfileMark(pm_gui, cvar_gui)
void cvar_gui(bool* pEnabled)
{
    ProfileBegin(pm_gui);
    EnsureInit();

    const u32 ldrPicker = ImGuiColorEditFlags_Float | ImGuiColorEditFlags_InputRGB;

    if (igBegin("Config Vars", pEnabled, 0))
    {
        cvar_t** cvars = ms_dict.values;
        const i32 length = ms_dict.count;
        const u32* indices = sdict_sort(&ms_dict, SDictStrCmp, NULL);

        igInputText("Search", ARGS(ms_search), 0, NULL, NULL);

        igSeparator();

        igExColumns(2);
        for (i32 i = 0; i < length; ++i)
        {
            u32 j = indices[i];
            cvar_t* cvar = cvars[j];
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
                    cvar_set_bool(cvar, cvar->asBool);
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
                    cvar_set_int(cvar, cvar->asInt);
                }
            }
            break;
            case cvart_float:
            {
                if (igExSliderFloat(cvar->name, &cvar->asFloat, cvar->minFloat, cvar->maxFloat))
                {
                    cvar_set_float(cvar, cvar->asFloat);
                }
            }
            break;
            case cvart_color:
            {
                if (igColorEdit4(cvar->name, &cvar->asVector.x, ldrPicker))
                {
                    cvar_set_vec(cvar, cvar->asVector);
                }
            }
            break;
            case cvart_point:
            {
                if (igExSliderFloat4(cvar->name, &cvar->asVector.x, cvar->minFloat, cvar->maxFloat))
                {
                    cvar_set_vec(cvar, cvar->asVector);
                }
            }
            break;
            case cvart_vector:
            {
                if (igExSliderFloat3(cvar->name, &cvar->asVector.x, -1.0f, 1.0f))
                {
                    cvar_set_vec(cvar, cvar->asVector);
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
