#include "rendering/rcmd.h"
#include "allocator/allocator.h"
#include <string.h>

static const i32 kCmdSize[] =
{
    0,
    sizeof(rcmd_clear_t),
    sizeof(rcmd_view_t),
    sizeof(rcmd_draw_t),
};

SASSERT(NELEM(kCmdSize) == RCmdType_COUNT);

rcmdbuf_t* rcmdbuf_create(void)
{
    rcmdbuf_t* buf = pim_calloc(EAlloc_Temp, sizeof(*buf));
    return buf;
}

bool rcmdbuf_read(const rcmdbuf_t* buf, i32* pCursor, rcmd_t* dst)
{
    i32 cmdType = RCmdType_NONE;

    ASSERT(pCursor);
    ASSERT(dst);

    const u8* buffer = buf->ptr;
    const i32 length = buf->length;
    i32 cursor = *pCursor;

    ASSERT(cursor >= 0);
    ASSERT(length >= 0);
    ASSERT(length < 0xffff);
    ASSERT(cursor <= length);

    if (cursor < length)
    {
        ASSERT((length - cursor) >= sizeof(cmdType));
        memcpy(&cmdType, buffer + cursor, sizeof(cmdType));
        cursor += sizeof(cmdType);

        ASSERT(cmdType > RCmdType_NONE);
        ASSERT(cmdType < RCmdType_COUNT);
        const i32 cmdSize = kCmdSize[cmdType];
        ASSERT((length - cursor) >= cmdSize);

        dst->type = cmdType;
        memcpy(&(dst->cmd), buffer + cursor, cmdSize);
        cursor += cmdSize;

        *pCursor = cursor;

        return true;
    }

    return false;
}

static void rcmdbuf_write(rcmdbuf_t* buf, i32 type, const void* src, i32 bytes)
{
    ASSERT(buf);
    ASSERT(src);
    ASSERT(bytes > 0);

    const i32 length = buf->length;
    void* ptr = buf->ptr;
    ptr = pim_realloc(EAlloc_Temp, ptr, length + sizeof(type) + bytes);

    u8* dst = ptr;
    dst += length;
    memcpy(dst, &type, sizeof(type));
    dst += sizeof(type);
    memcpy(dst, src, bytes);

    buf->ptr = ptr;
    buf->length = length + sizeof(type) + bytes;
}

void rcmd_clear(rcmdbuf_t* buf, u16 color, u16 depth)
{
    rcmd_clear_t cmd;
    cmd.color = color;
    cmd.depth = depth;
    rcmdbuf_write(buf, RCmdType_Clear, &cmd, sizeof(cmd));
}

void rcmd_view(rcmdbuf_t* buf, float4x4 view, float4x4 proj)
{
    rcmd_view_t cmd;
    cmd.V = view;
    cmd.P = proj;
    rcmdbuf_write(buf, RCmdType_View, &cmd, sizeof(cmd));
}

void rcmd_draw(rcmdbuf_t* buf, float4x4 model, mesh_t mesh, material_t material)
{
    rcmd_draw_t cmd;
    cmd.M = model;
    cmd.mesh = mesh;
    cmd.material = material;
    rcmdbuf_write(buf, RCmdType_Draw, &cmd, sizeof(cmd));
}

void rcmdqueue_create(rcmdqueue_t* queue)
{
    ASSERT(queue);
    ptrqueue_t* queues = queue->queues;
    for (i32 i = 0; i < kTileCount; ++i)
    {
        ptrqueue_create(queues + i, EAlloc_Perm, 256);
    }
}

void rcmdqueue_destroy(rcmdqueue_t* queue)
{
    ASSERT(queue);
    ptrqueue_t* queues = queue->queues;
    for (i32 i = 0; i < kTileCount; ++i)
    {
        ptrqueue_destroy(queues + i);
    }
}

void rcmdqueue_submit(rcmdqueue_t* queue, rcmdbuf_t* buf)
{
    ASSERT(queue);
    ASSERT(buf);
    ASSERT(buf->length >= 0);

    if (buf->length > 0)
    {
        for (i32 i = 0; i < kTileCount; ++i)
        {
            ptrqueue_t* q = &(queue->queues[i]);
            ptrqueue_push(q, buf);
        }
    }
}

rcmdbuf_t* rcmdqueue_read(rcmdqueue_t* queue, i32 tile_id)
{
    ASSERT(queue);
    ASSERT(tile_id < kTileCount);
    return ptrqueue_trypop(queue->queues + tile_id);
}
