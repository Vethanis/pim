#pragma once

#include "common/macro.h"

PIM_C_BEGIN

// ------------------------------------
// fd

typedef struct fd_s { i32 handle; } fd_t;

// only the fields we actually care about from stat
typedef struct fd_status_s
{
    i64 size;
    i64 accessTime;
    i64 modifyTime;
    i64 createTime;
} fd_status_t;

// ------------------------------------
// fmap

typedef struct FileMap_s
{
    void* ptr;
    i32 size;
    fd_t fd;
} FileMap;

// ------------------------------------
// fstr

typedef struct FStream_s
{
    void* handle;
} FStream;

// ------------------------------------
// dir

typedef enum
{
    ChMod_Read = 0x0100,
    ChMod_Write = 0x0080,
    ChMod_Exec = 0x0040,
} ChModFlags;

// ------------------------------------
// fnd

#if PLAT_WINDOWS

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

// Initialize to -1
typedef struct Finder_s
{
    isize handle;
    char path[PIM_PATH];
    char relPath[PIM_PATH];
    FinderData data;
} Finder;

#else

typedef struct Glob_s
{
    size_t gl_pathc;
    char **gl_pathv;
    size_t gl_offs;
} Glob;

typedef struct Finder_s
{
    Glob glob;
    bool open;
    i32 index;
    char* relPath;
} Finder;

#endif // PLAT_X

// ------------------------------------

PIM_C_END
