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
// sys.h -- non-portable functions

#include "interface/i_types.h"

typedef struct I_Sys_s
{
    // returns the file size
    // return -1 if file is not present
    // the file should be in BINARY mode
    i32(*_Sys_FileOpenRead)(const char *path, filehdl_t *hndl);
    filehdl_t(*_Sys_FileOpenWrite)(const char *path);
    void(*_Sys_FileClose)(filehdl_t handle);
    void(*_Sys_FileSeek)(filehdl_t handle, i32 position);
    i32(*_Sys_FileRead)(filehdl_t handle, void *dst, i32 count);
    i32(*_Sys_FileWrite)(filehdl_t handle, const void *src, i32 count);
    // returns 1 if file exists, -1 if it does not
    i32(*_Sys_FileTime)(const char *path);
    void(*_Sys_MkDir)(const char *path);
    void(*_Sys_DebugLog)(const char *file, const char *fmt, ...);
    // an error will cause the entire program to exit
    void(*_Sys_Error)(const char *error, ...);
    // send text to the console
    void(*_Sys_Printf)(const char *fmt, ...);
    void(*_Sys_Quit)(void);
    double(*_Sys_FloatTime)(void);
    char*(*_Sys_ConsoleInput)(void);
    // called to yield for 1ms so as not to hog cpu when paused or debugging
    void(*_Sys_Sleep)(void);
    // Perform Key_Event() callbacks until the input queue is empty
    void(*_Sys_SendKeyEvents)(void);
} I_Sys_t;
extern I_Sys_t I_Sys;

#define Sys_FileOpenRead(...) I_Sys._Sys_FileOpenRead(__VA_ARGS__)
#define Sys_FileOpenWrite(...) I_Sys._Sys_FileOpenWrite(__VA_ARGS__)
#define Sys_FileClose(...) I_Sys._Sys_FileClose(__VA_ARGS__)
#define Sys_FileSeek(...) I_Sys._Sys_FileSeek(__VA_ARGS__)
#define Sys_FileRead(...) I_Sys._Sys_FileRead(__VA_ARGS__)
#define Sys_FileWrite(...) I_Sys._Sys_FileWrite(__VA_ARGS__)
#define Sys_FileTime(...) I_Sys._Sys_FileTime(__VA_ARGS__)
#define Sys_MkDir(...) I_Sys._Sys_MkDir(__VA_ARGS__)
#define Sys_DebugLog(...) I_Sys._Sys_DebugLog(__VA_ARGS__)
#define Sys_Error(...) I_Sys._Sys_Error(__VA_ARGS__)
#define Sys_Printf(...) I_Sys._Sys_Printf(__VA_ARGS__)
#define Sys_Quit(...) I_Sys._Sys_Quit(__VA_ARGS__)
#define Sys_FloatTime(...) I_Sys._Sys_FloatTime(__VA_ARGS__)
#define Sys_ConsoleInput(...) I_Sys._Sys_ConsoleInput(__VA_ARGS__)
#define Sys_Sleep(...) I_Sys._Sys_Sleep(__VA_ARGS__)
#define Sys_SendKeyEvents(...) I_Sys._Sys_SendKeyEvents(__VA_ARGS__)
