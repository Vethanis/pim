#include "rendering/framebuffer.h"

#include "allocator/allocator.h"
#include "common/random.h"
#include "rendering/constants.h"

void framebuf_create(framebuf_t* buf, i32 width, i32 height)
{
    ASSERT(buf);
    ASSERT(width > 0);
    ASSERT(height > 0);
    buf->width = width;
    buf->height = height;
    const i32 len = width * height;
    buf->light = perm_malloc(len * sizeof(buf->light[0]));
    buf->color = perm_malloc(len * sizeof(buf->color[0]));
    buf->depth = perm_malloc(len * sizeof(buf->depth[0]));
    buf->tileFlags = perm_calloc(kTileCount * sizeof(buf->tileFlags[0]));
}

void framebuf_destroy(framebuf_t* buf)
{
    ASSERT(buf);
    buf->width = 0;
    buf->height = 0;
    pim_free(buf->light);
    buf->light = NULL;
    pim_free(buf->color);
    buf->color = NULL;
    pim_free(buf->depth);
    buf->depth = NULL;
    pim_free(buf->tileFlags);
    buf->tileFlags = NULL;
}

i32 framebuf_color_bytes(framebuf_t buf)
{
    return buf.width * buf.height * sizeof(buf.color[0]);
}

i32 framebuf_depth_bytes(framebuf_t buf)
{
    return buf.width * buf.height * sizeof(buf.depth[0]);
}
