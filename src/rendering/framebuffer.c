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

void framebuf_clear(framebuf_t buf, float c32f[4], float z32f)
{
    const int32_t len = buf.width * buf.height;
    const uint16_t c4 = f4_to_rgba4(c32f);
    const uint16_t z16 = f1_to_r16(z32f);
    for (int32_t i = 0; i < len; ++i)
    {
        buf.color[i] = c4;
    }
    for (int32_t i = 0; i < len; ++i)
    {
        buf.depth[i] = z16;
    }
}

int32_t framebuf_dtest_lte(framebuf_t buf, int32_t x, int32_t y, float z32f)
{
    const int32_t w = buf.width;
    const int32_t h = buf.height;
    if (x < 0 || y < 0 || x >= w || y >= h || z32f < 0.0f)
    {
        return 0;
    }
    const int32_t i = y * w + x;
    const uint16_t z16 = f1_to_r16(z32f);
    if (z16 <= buf.depth[i])
    {
        buf.depth[i] = z16;
        return 1;
    }
    return 0;
}

void framebuf_wcolor(framebuf_t buf, int32_t x, int32_t y, float c32f[4])
{
    const uint16_t c4 = f4_to_rgb5a1(c32f);
    const int32_t i = y * buf.width + x;
    buf.color[i] = c4;
}

void f4_dither(float x[4], float amt)
{
    const float kScale = 1.0f - amt;
    x[0] = x[0] * kScale + rand_float() * amt;
    x[1] = x[1] * kScale + rand_float() * amt;
    x[2] = x[2] * kScale + rand_float() * amt;
    x[3] = x[3] * kScale + rand_float() * amt;
}

uint32_t f4_to_rgba8(float x[4])
{
    uint32_t r = (uint32_t)(x[0] * 255.0f) & 255;
    uint32_t g = (uint32_t)(x[1] * 255.0f) & 255;
    uint32_t b = (uint32_t)(x[2] * 255.0f) & 255;
    uint32_t a = (uint32_t)(x[3] * 255.0f) & 255;
    uint32_t c = (a << 24) | (b << 16) | (g << 8) | (r);
    return c;
}

uint16_t f4_to_rgba4(float x[4])
{
    uint16_t r = (uint16_t)(x[0] * 15.0f) & 15;
    uint16_t g = (uint16_t)(x[1] * 15.0f) & 15;
    uint16_t b = (uint16_t)(x[2] * 15.0f) & 15;
    uint16_t a = (uint16_t)(x[3] * 15.0f) & 15;
    uint16_t c = (r << 12) | (g << 8) | (b << 4) | (a);
    return c;
}

uint16_t f4_to_rgb5a1(float x[4])
{
    uint16_t r = (uint16_t)(x[0] * 31.0f) & 31;
    uint16_t g = (uint16_t)(x[1] * 31.0f) & 31;
    uint16_t b = (uint16_t)(x[2] * 31.0f) & 31;
    uint16_t c = (r << 11) | (g << 6) | (b << 1);
    return c;
}

uint16_t f1_to_r16(float x)
{
    const float kMax = (1 << 16) - 1;
    return (uint16_t)(x * kMax);
}
