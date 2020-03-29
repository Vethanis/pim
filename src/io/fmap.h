#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include <stdint.h>
#include "io/fd.h"

typedef struct fmap_s
{
    void* ptr;
    int32_t size;
} fmap_t;

static int32_t fmap_isopen(fmap_t fmap) { return fmap.ptr != NULL; }

int32_t fmap_errno(void);

void fmap_create(fmap_t* fmap, fd_t fd, int32_t writable);
void fmap_destroy(fmap_t* map);
void fmap_flush(fmap_t map);

PIM_C_END
