#include "rendering/framebuffer.h"
#include "allocator/allocator.h"
#include <string.h>

void framebuf_create(framebuf_t* buf, i32 width, i32 height)
{
    ASSERT(buf);
    ASSERT(width > 0);
    ASSERT(height > 0);
    memset(buf, 0, sizeof(*buf));
    buf->width = width;
    buf->height = height;
    const i32 len = width * height;
    ASSERT(len > 0);
    buf->light = Perm_Alloc(len * sizeof(buf->light[0]));
    buf->color = Perm_Alloc(len * sizeof(buf->color[0]));
}

void framebuf_destroy(framebuf_t* buf)
{
    if (buf)
    {
        Mem_Free(buf->light);
        Mem_Free(buf->color);
        memset(buf, 0, sizeof(*buf));
    }
}
