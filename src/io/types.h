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

typedef struct Finder_s
{
    void* handle;
} Finder;

typedef struct FinderData_s
{
    char path[PIM_PATH];
    i64 size;
    i64 accessTime;
    i64 modifyTime;
    i64 createTime;
    u32 isNormal : 1;
    u32 isReadOnly : 1;
    u32 isHidden : 1;
    u32 isSystem : 1;
    u32 isFolder : 1;
    u32 isArchive : 1;
} FinderData;

// ------------------------------------

PIM_C_END
