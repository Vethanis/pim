#pragma once

#include "common/macro.h"
#include "io/fd.h"
#include "io/fstr.h"
#include "io/fmap.h"

PIM_C_BEGIN

typedef enum
{
    VFileType_Null = 0,
    VFileType_Descriptor,
    VFileType_Stream,
    VFileType_Map,
    VFileType_SubFile,
} VFileType;

typedef struct VFileHdl_s
{
    u32 version : 8;
    u32 index : 24;
} VFileHdl;

bool VFile_Exists(VFileHdl hdl);
bool VFile_IsOpen(VFileHdl hdl);
VFileHdl VFile_New(const char* path, const char* mode, VFileType type);
bool VFile_Del(VFileHdl hdl);
i32 VFile_Size(VFileHdl hdl);
i32 VFile_Read(VFileHdl hdl, i32 offset, i32 size, void* dst);
i32 VFile_Write(VFileHdl hdl, i32 offset, i32 size, const void* src);
VFileHdl SubFile_New(VFileHdl owner, i32 offset, i32 size, const char* name);

PIM_C_END
