#pragma once

#include "common/macro.h"

PIM_C_BEGIN

// Initialize to -1
typedef struct Finder_s
{
    isize handle;
} Finder;

typedef enum
{
    FAF_Normal = 0x00,
    FAF_ReadOnly = 0x01,
    FAF_Hidden = 0x02,
    FAF_System = 0x04,
    FAF_SubDir = 0x10,
    FAF_Archive = 0x20,
} FileAttrFlags;

// __finddata64_t, do not modify
typedef struct FinderData_s
{
    u32 attrib;
    i64 time_create;
    i64 time_access;
    i64 time_write;
    i64 size;
    char name[260];
} FinderData;

bool Finder_IsOpen(Finder fdr);
bool Finder_Begin(Finder* fdr, FinderData* data, const char* spec);
bool Finder_Next(Finder* fdr, FinderData* data);
// Always call End if you call Begin, or a leak occurs.
void Finder_End(Finder* fdr);

// Begin + Next + Close
bool Finder_Iterate(Finder* fdr, FinderData* data, const char* spec);

PIM_C_END
