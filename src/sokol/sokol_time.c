#include "common/macro.h"
#include "allocator/allocator.h"
#include "io/fd.h"

#define SOKOL_ASSERT(c)     ASSERT(c)
#define SOKOL_LOG(msg)      fd_puts(fd_stderr, msg)
#define SOKOL_MALLOC(s)     Perm_Alloc(s)
#define SOKOL_FREE(p)       Mem_Free(p)
#define SOKOL_IMPL          1

#include <sokol/sokol_time.h>
