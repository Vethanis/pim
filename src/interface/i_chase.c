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

#include "interface/i_chase.h"

#if QUAKE_IMPL

#include "interface/i_globals.h"
#include "interface/i_cvars.h"
#include "interface/i_world.h"
#include "interface/i_server.h"
#include "interface/i_mathlib.h"

void Chase_Init(void)
{

}

void Chase_Reset(void)
{
    // for respawning and teleporting
    // start position 12 units behind head
}

static void TraceLine(vec3_t start, vec3_t end, vec3_t impact)
{
    trace_t trace = { 0 };
    SV_RecursiveHullCheck(
        g_cl.worldmodel->hulls, 0, 0.0f, 1.0f, start, end, &trace);

    VectorCopy(trace.endpos, impact);
}

void Chase_Update(void)
{
    // if can't see player, reset
    vec3_t forward, up, right;
    AngleVectors(g_cl.viewangles, forward, right, up);

    // calc exact destination
    vec3_t chase_dest;
    for (i32 i = 0; i < 3; i++)
    {
        chase_dest[i] = g_r_refdef.vieworg[i]
            - forward[i] * cv_chase_back.value
            - right[i] * cv_chase_right.value;
    }
    chase_dest[2] = g_r_refdef.vieworg[2] + cv_chase_up.value;

    // find the spot the player is looking at
    vec3_t dest, stop;
    VectorMA(g_r_refdef.vieworg, 4096.0f, forward, dest);
    TraceLine(g_r_refdef.vieworg, dest, stop);

    // calculate pitch to look at the same spot from camera
    VectorSubtract(stop, g_r_refdef.vieworg, stop);
    float dist = DotProduct(stop, forward);
    dist = pim_max(dist, 1.0f);
    g_r_refdef.viewangles[PITCH] = -atanf(stop[2] / dist) / M_PI * 180.0f;

    // move towards destination
    VectorCopy(chase_dest, g_r_refdef.vieworg);
}

#endif // QUAKE_IMPL
