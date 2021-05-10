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
// vid.h -- video driver defs

#include "interface/i_types.h"

// called at startup and after any gamma correction
void VID_SetPalette(const u8 *palette);
// called for bonus and pain flashes, and for underwater color changes
void VID_ShiftPalette(const u8 *palette);
// Called at startup to set up translation tables, takes 256 8 bit RGB values
// the palette data will go away after the call, so it must be copied off if
// the video driver will need it again
void VID_Init(const u8 *palette);
// Called at shutdown
void VID_Shutdown(void);
// flushes the given rectangles from the view buffer to the screen
void VID_Update(vrect_t *rects);
// sets the mode; only used by the Quake engine for resetting to mode 0 (the
// base mode) on memory allocation failures
i32 VID_SetMode(i32 modenum, const u8 *palette);
// called only on Win32, when pause happens, so the mouse can be released
void VID_HandlePause(qboolean pause);
