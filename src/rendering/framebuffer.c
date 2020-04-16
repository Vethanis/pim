#include "rendering/framebuffer.h"

#include "allocator/allocator.h"
#include "common/random.h"

void framebuf_create(framebuf_t* buf, i32 width, i32 height)
{
    ASSERT(buf);
    ASSERT(width > 0);
    ASSERT(height > 0);
    buf->width = width;
    buf->height = height;
    const i32 len = width * height;
    buf->color = perm_malloc(len * sizeof(buf->color[0]));
    buf->depth = perm_malloc(len * sizeof(buf->depth[0]));
}

void framebuf_destroy(framebuf_t* buf)
{
    ASSERT(buf);
    buf->width = 0;
    buf->height = 0;
    pim_free(buf->color);
    buf->color = NULL;
    pim_free(buf->depth);
    buf->depth = NULL;
}

i32 framebuf_color_bytes(framebuf_t buf)
{
    return buf.width * buf.height * sizeof(buf.color[0]);
}

i32 framebuf_depth_bytes(framebuf_t buf)
{
    return buf.width * buf.height * sizeof(buf.depth[0]);
}

void framebuf_clear(framebuf_t buf, u32 color, float depth)
{
    const i32 len = buf.width * buf.height;
    for (i32 i = 0; i < len; ++i)
    {
        buf.color[i] = color;
    }
    for (i32 i = 0; i < len; ++i)
    {
        buf.depth[i] = depth;
    }
}
