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

void PR_Init(void);
void PR_ExecuteProgram(func_t fnum);
void PR_LoadProgs(void);
void PR_Profile_f(void);
void PR_RunError(const char* error, ...);

edict_t* ED_Alloc(void);
void ED_Free(edict_t *ed);
// returns a copy of the string allocated from the server's string heap
char*  ED_NewString(const char* string);
void ED_Print(edict_t *ed);
void ED_Write(filehdl_t f, edict_t *ed);
const char* ED_ParseEdict(const char* data, edict_t *ent);
void ED_WriteGlobals(filehdl_t f);
void ED_ParseGlobals(const char* data);
void ED_LoadFromFile(const char* data);
void ED_PrintEdicts(void);
void ED_PrintNum(i32 ent);
eval_t* GetEdictFieldValue(edict_t *ed, const char* field);

// link_t 'area' moved to top of edict_t
#define EDICT_FROM_AREA(l) ((edict_t*)(l))

#define EDICT_NUM(n) ((edict_t*)((u8*)g_sv.edicts + (n) * g_pr_edict_size))
#define NUM_FOR_EDICT(e) ((i32)(((u8*)(e) - (u8*)g_sv.edicts) / g_pr_edict_size))
#define NEXT_EDICT(e) ((edict_t*)((u8*)(e) + g_pr_edict_size))
#define EDICT_TO_PROG(e) ((i32)((u8*)(e) - (u8*)g_sv.edicts))
#define PROG_TO_EDICT(e) ((edict_t*)((u8*)g_sv.edicts + (e)))

#define G_FLOAT(o) (g_pr_globals[(o)])
#define G_INT(o) (*(i32*)&g_pr_globals[(o)])
#define G_EDICT(o) ((edict_t*)((u8*)g_sv.edicts + *(i32*)&g_pr_globals[(o)]))
#define G_EDICTNUM(o) NUM_FOR_EDICT(G_EDICT(o))
#define G_VECTOR(o) (&g_pr_globals[(o)])
#define G_STRING(o) (g_pr_strings + *(string_t*)&g_pr_globals[(o)])
#define G_FUNCTION(o) (*(func_t*)&g_pr_globals[(o)])

#define E_FLOAT(e,o) (((float*)&(e)->v)[(o)])
#define E_INT(e,o) (*(i32*)&((float*)&(e)->v)[(o)])
#define E_VECTOR(e,o) (&((float*)&(e)->v)[(o)])
#define E_STRING(e,o) (g_pr_strings + *(string_t*)&((float*)&(e)->v)[(o)])
