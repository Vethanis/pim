#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "io/fd.h"

typedef struct FileMap_s
{
    void* ptr;
    i32 size;
    fd_t fd;
} FileMap;

pim_inline bool FileMap_IsOpen(FileMap fmap) { return fmap.ptr != NULL; }

// memory maps the file descriptor
FileMap FileMap_New(fd_t fd, bool writable);

// does not close file descriptor
void FileMap_Del(FileMap* map);

// writes changes in mapped memory back to source
bool FileMap_Flush(FileMap map);

// owns descriptor
FileMap FileMap_Open(const char* path, bool writable);
// owns descriptor
void FileMap_Close(FileMap* map);

PIM_C_END
