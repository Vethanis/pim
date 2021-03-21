#include "allocator/allocator.h"

#define CGLTF_MALLOC(size)      Perm_Alloc((i32)(size))
#define CGLTF_FREE(ptr)         Mem_Free((ptr))

#define CGLTF_IMPLEMENTATION
#include "cgltf/cgltf.h"
