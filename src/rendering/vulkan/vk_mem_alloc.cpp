#include "common/macro.h"
#include <volk/volk.h>
#define VMA_ASSERT                      ASSERT
#define VMA_STATIC_VULKAN_FUNCTIONS     0
#define VMA_IMPLEMENTATION              1

#if COMPILER_CLANG
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wunused-variable"
#endif

#include "VulkanMemoryAllocator/include/vk_mem_alloc.h"
