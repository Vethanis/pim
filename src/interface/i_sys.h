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

void Sys_Init(i32 argc, const char** argv);
void Sys_SendKeyEvents(void);
const char* Sys_ConsoleInput(void);
bool Sys_FileIsOpen(filehdl_t handle);
i64 Sys_FileSize(filehdl_t handle);
// returns the file size
// return -1 if file is not present
// the file should be in BINARY mode
bool Sys_FileOpenRead(const char *path, filehdl_t* hdlOut);
bool Sys_FileOpenWrite(const char *path, filehdl_t* hdlOut);
void Sys_FileClose(filehdl_t handle);
bool Sys_FileSeek(filehdl_t handle, i32 position);
i32 Sys_FileRead(filehdl_t handle, void *dst, i32 count);
i32 Sys_FileWrite(filehdl_t handle, const void *src, i32 count);
i32 Sys_FileVPrintf(filehdl_t handle, const char* fmt, va_list ap);
i32 Sys_FilePrintf(filehdl_t handle, const char* fmt, ...);
i32 Sys_FileVScanf(filehdl_t handle, const char* fmt, va_list ap);
i32 Sys_FileScanf(filehdl_t handle, const char* fmt, ...);
bool Sys_FileAtEnd(filehdl_t handle);
i32 Sys_FileGetc(filehdl_t handle);
// returns >= 0 if file exists, -1 if it does not
i64 Sys_FileTime(const char *path);
bool Sys_MkDir(const char *path);
// an error will cause the entire program to exit
void Sys_Error(const char *error, ...);
// send text to the console
void Sys_Printf(const char *fmt, ...);
void Sys_DPrintf(const char *fmt, ...);
void Sys_Quit(void);
double Sys_FloatTime(void);
// called to yield for 1ms so as not to hog cpu when paused or debugging
void Sys_Sleep(void);
