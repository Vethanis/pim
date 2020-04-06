#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "io/fd.h"

typedef struct fmap_s
{
    void* ptr;
    i32 size;
} fmap_t;

static i32 fmap_isopen(fmap_t fmap) { return fmap.ptr != NULL; }

i32 fmap_errno(void);

void fmap_create(fmap_t* fmap, fd_t fd, i32 writable);
void fmap_destroy(fmap_t* map);
void fmap_flush(fmap_t map);

PIM_C_END
