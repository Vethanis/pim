#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "io/fd.h"

typedef struct fmap_s
{
    void* ptr;
    i32 size;
    fd_t fd;
} fmap_t;

static bool fmap_isopen(fmap_t fmap) { return fmap.ptr != NULL; }

// memory maps the file descriptor
fmap_t fmap_create(fd_t fd, bool writable);

// does not close file descriptor
void fmap_destroy(fmap_t* map);

// writes changes in mapped memory back to source
bool fmap_flush(fmap_t map);

PIM_C_END
