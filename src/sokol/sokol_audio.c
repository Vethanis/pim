#include "common/macro.h"
#include "allocator/allocator.h"
#include "io/fd.h"

#define SOKOL_ASSERT(c)     ASSERT(c)
#define SOKOL_LOG(msg)      fd_puts(msg, fd_stderr)
#define SOKOL_MALLOC(s)     perm_malloc(s)
#define SOKOL_FREE(p)       pim_free(p)
#define SOKOL_IMPL          1

#include <sokol/sokol_audio.h>
