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

#include "common/macro.h"
#include "io/fmap.h"

// dpackheader_t
// common.c 1231
// must match exactly for binary compatibility
typedef struct dpackheader_s
{
    char id[4]; // "PACK"
    i32 offset;
    i32 length;
} dpackheader_t;

// dpackfile_t
// common.c 1225
// must match exactly for binary compatibility
typedef struct dpackfile_s
{
    char name[56];
    i32 offset;
    i32 length;
} dpackfile_t;

typedef struct Pack_s
{
    char path[PIM_PATH];
    FileMap mapped;
    i32 filecount;
    const dpackfile_t* files;
} Pack;

typedef struct SearchPath_s
{
    i32 packCount;
    Pack* packs;
    i32 fileCount;
    char** filenames;
} SearchPath;

bool Pack_Load(Pack* pack, const char* dir);
void Pack_Free(Pack* pack);

void SearchPath_New(SearchPath* sp);
void SearchPath_Del(SearchPath* sp);
i32 SearchPath_AddPack(SearchPath* sp, const char* dir);
bool SearchPath_RmPack(SearchPath* sp, const char* dir);
i32 SearchPath_FindPack(SearchPath* sp, const char* dir);
