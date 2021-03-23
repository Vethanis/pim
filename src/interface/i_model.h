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

typedef struct I_Model_s
{
    void(*_Mod_Init)(void);
    void(*_Mod_ClearAll)(void);
    model_t*(*_Mod_ForName)(const char *name, qboolean crash);
    void*(*_Mod_Extradata)(model_t *mod); // handles caching
    void(*_Mod_TouchModel)(const char *name);
    mleaf_t*(*_Mod_PointInLeaf)(const float *p, const model_t *model);
    const u8*(*_Mod_LeafPVS)(const mleaf_t *leaf, const model_t *model);
} I_Model_t;
extern I_Model_t I_Model;

#define Mod_Init(...) I_Model._Mod_Init(__VA_ARGS__)
#define Mod_ClearAll(...) I_Model._Mod_ClearAll(__VA_ARGS__)
#define Mod_ForName(...) I_Model._Mod_ForName(__VA_ARGS__)
#define Mod_Extradata(...) I_Model._Mod_Extradata(__VA_ARGS__)
#define Mod_TouchModel(...) I_Model._Mod_TouchModel(__VA_ARGS__)
#define Mod_PointInLeaf(...) I_Model._Mod_PointInLeaf(__VA_ARGS__)
#define Mod_LeafPVS(...) I_Model._Mod_LeafPVS(__VA_ARGS__)
