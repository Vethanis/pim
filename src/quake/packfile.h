#pragma once

#include "common/macro.h"

PIM_C_BEGIN

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

typedef struct pack_s
{
    char path[PIM_PATH];
    const dpackheader_t* header;
    const dpackfile_t* files;
    i32 filecount;
    i32 bytes;
} pack_t;

typedef struct folder_s
{
    char path[PIM_PATH];
    pack_t* packs;
    i32 length;
} folder_t;

pack_t pack_load(const char* dir, EAlloc allocator);
void pack_free(pack_t* pack);

folder_t folder_load(const char* dir, EAlloc allocator);
void folder_free(folder_t* folder);

PIM_C_END
