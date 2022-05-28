#include "rendering/vulkan/vkr_context.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_desc.h"
#include "rendering/vulkan/vkr_mem.h"
#include "rendering/vulkan/vkr_buffer.h"
#include "allocator/allocator.h"
#include "common/profiler.h"
#include "threading/task.h"
#include <string.h>

vkrContext* vkrGetContext(void)
{
    return &g_vkr.context;
}

bool vkrContext_New(vkrContext* ctx)
{
    memset(ctx, 0, sizeof(*ctx));
    return true;
}

void vkrContext_Del(vkrContext* ctx)
{
    memset(ctx, 0, sizeof(*ctx));
}
