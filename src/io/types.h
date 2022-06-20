#pragma once

#include "common/macro.h"

PIM_C_BEGIN

// ------------------------------------
// fd

typedef struct fd_s { i32 handle; } fd_t;

typedef enum
{
    StMode_Regular = 0x8000,
    StMode_Dir = 0x4000,
    StMode_SpecChar = 0x2000,
    StMode_Pipe = 0x1000,
    StMode_Read = 0x0100,
    StMode_Write = 0x0080,
    StMode_Exec = 0x0040,
} StModeFlags;

#if PLAT_LINUX

// FIXME: don't import, find a way to properly represent this struct.
#include <sys/stat.h>
typedef struct stat fd_status_t;

#else

typedef struct fd_status_s
{
    u32 st_dev;
    u16 st_ino;
    u16 st_mode;
    i16 st_nlink;
    i16 st_uid;
    i16 st_guid;
    u32 st_rdev;
    i64 st_size;
    i64 st_atime;
    i64 st_mtime;
    i64 st_ctime;
} fd_status_t;

#endif // PLAT_LINUX

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

// Initialize to -1
typedef struct Finder_s
{
    isize handle;
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
} Finder;

#endif // PLAT_WINDOWS

typedef enum
{
    FAF_Normal = 0x00,
    FAF_ReadOnly = 0x01,
    FAF_Hidden = 0x02,
    FAF_System = 0x04,
    FAF_SubDir = 0x10,
    FAF_Archive = 0x20,
} FileAttrFlags;

#if PLAT_WINDOWS

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

#else

typedef struct FinderData_s
{
    // FIXME: on windows it's just the name, linux it's the full path. grumble.
    char* name;
} FinderData;

#endif // PLAT_WINDOWS

// ------------------------------------

PIM_C_END
