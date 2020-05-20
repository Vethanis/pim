#include "common/cvar.h"

#include "allocator/allocator.h"
#include "common/fnv1a.h"
#include "common/stringutil.h"
#include "common/profiler.h"
#include "common/sort.h"
#include "containers/sdict.h"
#include "ui/cimgui.h"
#include <stdlib.h> // atof

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
    ASSERT(ptr->description);
    ptr->asFloat = (float)atof(ptr->value);

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
    StrCpy(ARGS(ptr->value), value);
    if (ptr->type != cvart_text)
    {
        ptr->asFloat = (float)atof(value);
    }
    ptr->flags |= cvarf_dirty;
}

void cvar_set_float(cvar_t* ptr, float value)
{
    ASSERT(ptr);
    SPrintf(ARGS(ptr->value), "%f", value);
    ptr->asFloat = value;
    ptr->flags |= cvarf_dirty;
}

bool cvar_check_dirty(cvar_t* ptr)
{
    ASSERT(ptr);
    if (ptr->flags & cvarf_dirty)
    {
        ptr->flags &= ~cvarf_dirty;
        return true;
    }
    return false;
}

static char ms_search[PIM_PATH];

ProfileMark(pm_gui, cvar_gui)
void cvar_gui(bool* pEnabled)
{
    ProfileBegin(pm_gui);
    EnsureInit();

    if (igBegin("Config Vars", pEnabled, 0))
    {
        cvar_t** cvars = ms_dict.values;
        const i32 length = ms_dict.count;
        const u32* indices = sdict_sort(&ms_dict, SDictStrCmp, NULL);

        igInputText("Search", ARGS(ms_search), 0, NULL, NULL);

        igSeparator();

        igColumns(2);
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
                bool value = cvar->asFloat != 0.0f;
                if (igCheckbox(cvar->name, &value))
                {
                    cvar_set_float(cvar, value ? 1.0f : 0.0f);
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
                i32 value = (i32)cvar->asFloat;
                if (igSliderInt(cvar->name, &value, 0, 1000, "%d"))
                {
                    cvar_set_float(cvar, (float)value);
                }
            }
            break;
            case cvart_float:
            {
                float value = cvar->asFloat;
                if (igSliderFloat(cvar->name, &value, -10.0f, 10.0f))
                {
                    cvar_set_float(cvar, value);
                }
            }
            break;
            }
            igNextColumn();
            igText(cvar->description); igNextColumn();
        }
        igColumns(1);
    }
    igEnd();

    ProfileEnd(pm_gui);
}
