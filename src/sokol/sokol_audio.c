#include "common/macro.h"
#include "allocator/allocator.h"
#include "io/fd.h"

#define SOKOL_ASSERT(c)     ASSERT(c)
#define SOKOL_LOG(msg)      fd_puts(fd_stderr, msg)
#define SOKOL_IMPL          1

#include <sokol/sokol_audio.h>
