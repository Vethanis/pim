#pragma once

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

#include "interface/i_types.h"

typedef struct I_Progs_s
{
    void(*_PR_Init)(void);
    void(*_PR_ExecuteProgram)(func_t fnum);
    void(*_PR_LoadProgs)(void);
    void(*_PR_Profile_f)(void);
    void(*_PR_RunError)(const char* error, ...);

    edict_t* (*_ED_Alloc)(void);
    void(*_ED_Free)(edict_t *ed);
    // returns a copy of the string allocated from the server's string heap
    char* (*_ED_NewString)(const char* string);
    void(*_ED_Print)(edict_t *ed);
    void(*_ED_Write)(filehdl_t f, edict_t *ed);
    const char* (*_ED_ParseEdict)(const char* data, edict_t *ent);
    void(*_ED_WriteGlobals)(filehdl_t f);
    void(*_ED_ParseGlobals)(const char* data);
    void(*_ED_LoadFromFile)(const char* data);
    void(*_ED_PrintEdicts)(void);
    void(*_ED_PrintNum)(i32 ent);
    eval_t* (*_GetEdictFieldValue)(edict_t *ed, const char* field);
} I_Progs_t;
extern I_Progs_t I_Progs;

#define PR_Init(...) I_Progs._PR_Init(__VA_ARGS__)
#define PR_ExecuteProgram(...) I_Progs._PR_ExecuteProgram(__VA_ARGS__)
#define PR_LoadProgs(...) I_Progs._PR_LoadProgs(__VA_ARGS__)
#define PR_Profile_f(...) I_Progs._PR_Profile_f(__VA_ARGS__)
#define PR_RunError(...) I_Progs._PR_RunError(__VA_ARGS__)

#define ED_Alloc(...) I_Progs._ED_Alloc(__VA_ARGS__)
#define ED_Free(...) I_Progs._ED_Free(__VA_ARGS__)
#define ED_NewString(...) I_Progs._ED_NewString(__VA_ARGS__)
#define ED_Print(...) I_Progs._ED_Print(__VA_ARGS__)
#define ED_Write(...) I_Progs._ED_Write(__VA_ARGS__)
#define ED_ParseEdict(...) I_Progs._ED_ParseEdict(__VA_ARGS__)
#define ED_WriteGlobals(...) I_Progs._ED_WriteGlobals(__VA_ARGS__)
#define ED_ParseGlobals(...) I_Progs._ED_ParseGlobals(__VA_ARGS__)
#define ED_LoadFromFile(...) I_Progs._ED_LoadFromFile(__VA_ARGS__)
#define ED_PrintEdicts(...) I_Progs._ED_PrintEdicts(__VA_ARGS__)
#define ED_PrintNum(...) I_Progs._ED_PrintNum(__VA_ARGS__)
#define GetEdictFieldValue(...) I_Progs._GetEdictFieldValue(__VA_ARGS__)

// link_t 'area' moved to top of edict_t
#define EDICT_FROM_AREA(l) ((edict_t*)(l))

#define EDICT_NUM(n) ((edict_t*)((u8*)sv.edicts + (n) * pr_edict_size))
#define NUM_FOR_EDICT(e) (((u8*)(e) - (u8*)sv.edicts) / pr_edict_size)
#define NEXT_EDICT(e) ((edict_t*)((u8*)(e) + pr_edict_size))
#define EDICT_TO_PROG(e) ((u8*)(e) - (u8*)sv.edicts)
#define PROG_TO_EDICT(e) ((edict_t*)((u8*)sv.edicts + (e)))

#define G_FLOAT(o) (pr_globals[(o)])
#define G_INT(o) (*(i32*)&pr_globals[(o)])
#define G_EDICT(o) ((edict_t*)((u8*)sv.edicts + *(i32*)&pr_globals[(o)]))
#define G_EDICTNUM(o) NUM_FOR_EDICT(G_EDICT(o))
#define G_VECTOR(o) (&pr_globals[(o)])
#define G_STRING(o) (pr_strings + *(string_t*)&pr_globals[(o)])
#define G_FUNCTION(o) (*(func_t*)&pr_globals[(o)])

#define E_FLOAT(e,o) (((float*)&(e)->v)[(o)])
#define E_INT(e,o) (*(i32*)&((float*)&(e)->v)[(o)])
#define E_VECTOR(e,o) (&((float*)&(e)->v)[(o)])
#define E_STRING(e,o) (pr_strings + *(string_t*)&((float*)&(e)->v)[(o)])
