#include "common/cvar.h"

#include "allocator/allocator.h"
#include "common/fnv1a.h"
#include "common/stringutil.h"
#include "common/profiler.h"
#include "containers/dict.h"
#include "containers/text.h"
#include "ui/cimgui.h"
#include <stdlib.h> // atof

static dict_t ms_dict;

static void EnsureInit(void)
{
    if (!ms_dict.valueSize)
    {
        dict_new(&ms_dict, sizeof(text32), sizeof(cvar_t*), EAlloc_Perm);
    }
}

void cvar_reg(cvar_t* ptr)
{
    EnsureInit();

    ASSERT(ptr);
    ASSERT(ptr->name);
    ASSERT(ptr->description);
    ptr->asFloat = (float)atof(ptr->value);

    text32 txt;
    text_new(&txt, sizeof(txt), ptr->name);
    bool added = dict_add(&ms_dict, &txt, &ptr);
    ASSERT(added);
}

cvar_t* cvar_find(const char* name)
{
    EnsureInit();

    ASSERT(name);
    text32 txt;
    text_new(&txt, sizeof(txt), name);
    const i32 i = dict_find(&ms_dict, &txt);
    if (i != -1)
    {
        cvar_t** cvars = ms_dict.values;
        return cvars[i];
    }
    return NULL;
}

const char* cvar_complete(const char* namePart)
{
    EnsureInit();

    ASSERT(namePart);
    const i32 partLen = StrLen(namePart);
    const u32 width = ms_dict.width;
    const text32* names = ms_dict.keys;
    for (u32 i = 0; i < width; ++i)
    {
        const char* name = names[i].c;
        if (name[0] && !StrCmp(namePart, partLen, name))
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

ProfileMark(pm_gui, cvar_gui)
void cvar_gui(bool* pEnabled)
{
    ProfileBegin(pm_gui);
    EnsureInit();

    if (igBegin("Config Vars", pEnabled, 0))
    {
        const u32 width = ms_dict.width;
        cvar_t** cvars = ms_dict.values;

        igColumns(2);
        for (u32 i = 0; i < width; ++i)
        {
            cvar_t* cvar = cvars[i];
            if (!cvar)
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
                if (igInputText(cvar->name, ARGS(cvar->value), 0, NULL, NULL) && (StrLen(cvar->value) > 0))
                {
                    cvar->asFloat = (float)atof(cvar->value);
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
            igText(cvars[i]->description); igNextColumn();
        }
        igColumns(1);
    }
    igEnd();

    ProfileEnd(pm_gui);
}
