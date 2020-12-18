#include "allocator/allocator.h"

#define CGLTF_MALLOC(size)      perm_malloc((i32)(size))
#define CGLTF_FREE(ptr)         pim_free((ptr))

#define CGLTF_IMPLEMENTATION
#include "cgltf/cgltf.h"
