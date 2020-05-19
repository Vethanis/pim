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

    i32 w = width;
    i32 h = height;
    i32* offsets = NULL;
    i32 i = 0;
    i32 m = 0;
    while ((w * h) > 1)
    {
        ++m;
        offsets = perm_realloc(offsets, sizeof(offsets[0]) * m);
        offsets[m - 1] = i;
        i += w * h;
        w = w >> 1;
        h = h >> 1;
        w = w > 1 ? w : 1;
        h = h > 1 ? h : 1;
    }
    buf->depth = perm_malloc(i * sizeof(buf->depth[0]));
    buf->mipCount = m;
    buf->offsets = offsets;
}

void framebuf_destroy(framebuf_t* buf)
{
    ASSERT(buf);
    buf->width = 0;
    buf->height = 0;
    buf->mipCount = 0;
    pim_free(buf->light);
    buf->light = NULL;
    pim_free(buf->color);
    buf->color = NULL;
    pim_free(buf->depth);
    buf->depth = NULL;
    pim_free(buf->offsets);
    buf->offsets = NULL;
}

i32 framebuf_color_bytes(framebuf_t buf)
{
    return buf.width * buf.height * sizeof(buf.color[0]);
}
