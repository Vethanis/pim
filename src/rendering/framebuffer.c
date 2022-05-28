#include "rendering/framebuffer.h"
#include "allocator/allocator.h"
#include <string.h>

void FrameBuf_New(FrameBuf* buf, i32 width, i32 height)
{
    ASSERT(buf);
    ASSERT(width > 0);
    ASSERT(height > 0);
    memset(buf, 0, sizeof(*buf));
    buf->width = width;
    buf->height = height;
    const i32 len = width * height;
    ASSERT(len > 0);
    buf->light = Tex_Alloc(len * sizeof(buf->light[0]));
}

void FrameBuf_Del(FrameBuf* buf)
{
    if (buf)
    {
        Mem_Free(buf->light);
        memset(buf, 0, sizeof(*buf));
    }
}

void FrameBuf_Reserve(FrameBuf* buf, i32 width, i32 height)
{
    const i32 len = width * height;
    if (len > 0)
    {
        buf->light = Tex_Realloc(buf->light, sizeof(buf->light[0]) * len);
        buf->width = width;
        buf->height = height;
    }
}
