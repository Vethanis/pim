#include "rendering/framebuffer.h"

#include "allocator/allocator.h"
#include "common/random.h"

void framebuf_create(framebuf_t* buf, int32_t width, int32_t height)
{
    ASSERT(buf);
    ASSERT(width > 0);
    ASSERT(height > 0);
    buf->width = width;
    buf->height = height;
    const int32_t len = width * height;
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

int32_t framebuf_color_bytes(framebuf_t buf)
{
    return buf.width * buf.height * sizeof(buf.color[0]);
}

int32_t framebuf_depth_bytes(framebuf_t buf)
{
    return buf.width * buf.height * sizeof(buf.depth[0]);
}

void framebuf_clear(framebuf_t buf, uint16_t color, uint16_t depth)
{
    const int32_t len = buf.width * buf.height;
    for (int32_t i = 0; i < len; ++i)
    {
        buf.color[i] = color;
    }
    for (int32_t i = 0; i < len; ++i)
    {
        buf.depth[i] = depth;
    }
}
