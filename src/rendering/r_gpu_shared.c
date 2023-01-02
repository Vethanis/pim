#include "r_gpu_shared.h"

SASSERT((sizeof(GpuGlobals) % 16) == 0);
GpuGlobals g_GpuGlobals;
