/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "interface/i_cvar.h"

#if QUAKE_IMPL

#include "interface/i_cmd.h"
#include "interface/i_console.h"
#include "interface/i_server.h"
#include "interface/i_globals.h"
#include "interface/i_sys.h"

#include "containers/sdict.h"
#include "common/stringutil.h"
#include "allocator/allocator.h"

#include <string.h>
#include <math.h>

static StrDict ms_dict =
{
    .valueSize = sizeof(void*),
};

void Cvar_RegisterVariable(cvar_t *var)
{
    ASSERT(var);
    ASSERT(var->name && var->name[0]);
    ASSERT(var->defaultValue && var->defaultValue[0]);
    if (var && var->name && var->defaultValue)
    {
        if (Cmd_Exists(var->name))
        {
            Con_Printf("Cvar_RegisterVariable: %s is a command\n", var->name);
            return;
        }

        Mem_Free(var->stringValue);
        var->stringValue = StrDup(var->defaultValue, EAlloc_Perm);

        if (!StrDict_Add(&ms_dict, var->name, &var))
        {
            Con_Printf("Can't register variable %s, already defined\n", var->name);
        }
    }
}

void Cvar_Set(const char *name, const char *value)
{
    ASSERT(name && name[0]);
    ASSERT(value && value[0]);
    if (name && value)
    {
        cvar_t* var = Cvar_FindVar(name);
        if (!var)
        {
            Con_Printf("Cvar_Set: variable %s not found\n", name);
            return;
        }

        bool changed = StrCmp(var->stringValue, PIM_PATH, value) != 0;

        Mem_Free(var->stringValue);
        var->stringValue = StrDup(value, EAlloc_Perm);
        var->value = (float)atof(value);

        if (var->server && changed)
        {
            if (g_sv.active)
            {
                SV_BroadcastPrintf("\"%s\" changed to \"%s\"\n", var->name, value);
            }
        }
    }
}

void Cvar_SetValue(const char *name, float value)
{
    ASSERT(name);
    ASSERT(isfinite(value));
    if (name)
    {
        char valueStr[32];
        SPrintf(ARGS(valueStr), "%f", value);
        Cvar_Set(name, valueStr);
    }
}

float Cvar_VariableValue(const char *name)
{
    const cvar_t* var = Cvar_FindVar(name);
    if (var)
    {
        ASSERT(isfinite(var->value));
        return var->value;
    }
    return 0.0f;
}

const char* Cvar_VariableString(const char *name)
{
    const cvar_t* var = Cvar_FindVar(name);
    if (var)
    {
        ASSERT(var->stringValue);
        return var->stringValue;
    }
    return "";
}

const char* Cvar_CompleteVariable(const char *partial)
{
    ASSERT(partial);
    if (partial && partial[0])
    {
        const i32 partLen = StrLen(partial);
        const u32 width = ms_dict.width;
        const char** names = ms_dict.keys;
        for (u32 i = 0; i < width; ++i)
        {
            const char* name = names[i];
            if (name && !StrCmp(partial, partLen, name))
            {
                return name;
            }
        }
    }
    return NULL;
}

qboolean Cvar_Command(void)
{
    const cvar_t* v = Cvar_FindVar(Cmd_Argv(0));
    if (v)
    {
        if (Cmd_Argc() == 1)
        {
            Con_Printf("\"%s\" is \"%s\"\n", v->name, v->stringValue);
            return true;
        }
        Cvar_Set(v->name, Cmd_Argv(1));
        return true;
    }
    return false;
}

void Cvar_WriteVariables(filehdl_t file)
{
    if (Sys_FileIsOpen(file))
    {
        const u32 width = ms_dict.width;
        const cvar_t** vars = ms_dict.values;
        for (u32 i = 0; i < width; ++i)
        {
            const cvar_t* var = vars[i];
            if (var)
            {
                Sys_FilePrintf(file, "%s \"%s\"\n", var->name, var->stringValue);
            }
        }
    }
}

cvar_t* Cvar_FindVar(const char *name)
{
    cvar_t* var = NULL;
    StrDict_Get(&ms_dict, name, &var);
    return var;
}

#endif // QUAKE_IMPL
