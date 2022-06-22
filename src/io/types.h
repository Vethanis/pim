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

#if PLAT_WINDOWS

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

#else

typedef struct fd_timespec_s
{
    i64 tv_sec;
    i64 tv_nsec;
} fd_timespec_t;

// Likely not very portable, based on GNU c.
typedef struct fd_status_s
{
    u32 st_dev;
    u64 st_ino;
    u64 st_nlink;
    u32 st_mode;
    u32 st_uid;
    u32 st_gid;
    i32 __pad0;
    u64 st_rdev;
    u64 st_size;
    u64 st_blksize;
    u64 st_blocks;
    i64 __reserved[3];
    struct fd_timespec_s st_atim;
    struct fd_timespec_s st_mtim;
    struct fd_timespec_s st_ctim;
#   define st_atime st_atim.tv_sec
#   define st_mtime st_mtim.tv_sec
#   define st_ctime st_ctim.tv_sec
} fd_status_t;

#endif // PLAT_X

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
