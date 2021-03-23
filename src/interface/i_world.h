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
// world.h

#include "interface/i_types.h"

typedef struct I_World_s
{
    // called after the world model has been loaded, before linking any entities
    void(*_SV_ClearWorld)(void);
    // call before removing an entity, and before trying to move one,
    // so it doesn't clip against itself
    // flags ent->v.modified
    void(*_SV_UnlinkEdict)(edict_t *ent);
    // Needs to be called any time an entity changes origin, mins, maxs, or solid
    // flags ent->v.modified
    // sets ent->v.absmin and ent->v.absmax
    // if touchtriggers, calls prog functions for the intersected triggers
    void(*_SV_LinkEdict)(edict_t *ent, qboolean touch_triggers);
    i32(*_SV_PointContents)(vec3_t p);
    // returns the CONTENTS_* value from the world at the given point.
    // does not check any entities at all
    // the non-true version remaps the water current contents to content_water
    i32(*_SV_TruePointContents)(vec3_t p);
    edict_t*(*_SV_TestEntityPosition)(edict_t *ent);
    // mins and maxs are relative
    // if the entire move stays in a solid volume, trace.allsolid will be set
    // if the starting point is in a solid, it will be allowed to move out
    // to an open area
    // nomonsters is used for line of sight or edge testing, where mosnters
    // shouldn't be considered solid objects
    // passedict is explicitly excluded from clipping checks (normally NULL)
    trace_t(*_SV_Move)(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, i32 type, edict_t *passedict);
} I_World_t;
extern I_World_t I_World;

#define SV_ClearWorld(...) I_World._SV_ClearWorld(__VA_ARGS__)
#define SV_UnlinkEdict(...) I_World._SV_UnlinkEdict(__VA_ARGS__)
#define SV_LinkEdict(...) I_World._SV_LinkEdict(__VA_ARGS__)
#define SV_PointContents(...) I_World._SV_PointContents(__VA_ARGS__)
#define SV_TruePointContents(...) I_World._SV_TruePointContents(__VA_ARGS__)
#define SV_TestEntityPosition(...) I_World._SV_TestEntityPosition(__VA_ARGS__)
#define SV_Move(...) I_World._SV_Move(__VA_ARGS__)
