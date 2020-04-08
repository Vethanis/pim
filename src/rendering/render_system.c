#include "rendering/render_system.h"

#include <sokol/sokol_gfx.h>
#include <sokol/sokol_app.h>
#include <sokol/util/sokol_imgui.h>
#include <sokol/util/sokol_gl.h>
#include "ui/cimgui.h"

#include "allocator/allocator.h"
#include "common/time.h"
#include "components/ecs.h"
#include "rendering/framebuffer.h"
#include "rendering/constants.h"
#include "rendering/rcmd.h"
#include "math/float4_funcs.h"
#include "math/float2_funcs.h"
#include "math/float4x4.h"
#include "math/int2.h"
#include "math/int4.h"
#include "common/random.h"
#include "containers/idset.h"
#include "containers/ptrqueue.h"
#include <string.h>

typedef struct rastertask_s
{
    task_t task;
    i32 iFrame;
} rastertask_t;

typedef struct cmdtask_s
{
    ecs_foreach_t task;
} cmdtask_t;

typedef struct renderstate_s
{
    float4 viewport;
    float4x4 P;
    float4x4 V;
    i32 iTile;
    i32 iFrame;
} renderstate_t;

static i32 ms_width;
static i32 ms_height;
static sg_features ms_features;
static sg_limits ms_limits;
static sg_backend ms_backend;
static i32 ms_iFrame;
static sg_image ms_images[kNumFrames];
static framebuf_t ms_buffers[kNumFrames];
static rcmdqueue_t ms_queues[kNumFrames];
static rastertask_t ms_rastasks[kNumFrames];
static cmdtask_t ms_cmdtasks[kNumFrames];

math_inline float4 VEC_CALL f4_rand(void)
{
    return f4_v(rand_float(), rand_float(), rand_float(), rand_float());
}

math_inline float2 VEC_CALL f2_rand(void)
{
    return f2_v(rand_float(), rand_float());
}

math_inline u16 VEC_CALL f4_rgb5a1(float4 v)
{
    u16 r = (u16)(v.x * 31.0f) & 31;
    u16 g = (u16)(v.y * 31.0f) & 31;
    u16 b = (u16)(v.z * 31.0f) & 31;
    u16 c = (r << 11) | (g << 6) | (b << 1) | 1;
    return c;
}

math_inline int4 VEC_CALL GetScreenTile(i32 i)
{
    i32 x = (i & 7) * kTileWidth;
    i32 y = (i >> 3) * kTileHeight;
    return (int4) { x, y, kTileWidth, kTileHeight };
}

math_inline float4 VEC_CALL GetClipTile(i32 i)
{
    const float dx = 1.0f / kDrawWidth;
    const float dy = 1.0f / kDrawHeight;
    int4 sc = GetScreenTile(i);
    float4 cl = { sc.x * dx, sc.y * dy, sc.z * dx, sc.w * dy };
    return cl;
}

static void VEC_CALL ClearTile(renderstate_t state, rcmd_clear_t clear)
{
    framebuf_t frame = ms_buffers[state.iFrame];
    const int4 tile = GetScreenTile(state.iTile);
    for (i32 ty = tile.y; ty < tile.w; ++ty)
    {
        for (i32 tx = tile.x; tx < tile.z; ++tx)
        {
            i32 i = kDrawWidth * ty + tx;
            frame.color[i] = clear.color;
        }
    }
    for (i32 ty = tile.y; ty < tile.w; ++ty)
    {
        for (i32 tx = tile.x; tx < tile.z; ++tx)
        {
            i32 i = kDrawWidth * ty + tx;
            frame.depth[i] = clear.depth;
        }
    }
}

math_inline float4 VEC_CALL f4_blend(float4 a, float4 b, float4 c, float2 blend)
{
    float4 y = a;
    y = f4_add(y, f4_mul(b, f4_s(blend.x)));
    y = f4_add(y, f4_mul(c, f4_s(blend.y)));
    return y;
}

static void VEC_CALL DrawMesh(renderstate_t state, rcmd_draw_t draw)
{
    framebuf_t frame = ms_buffers[state.iFrame];
    float4 tile = GetClipTile(state.iTile);
    // float4x4 M = draw.M;
    mesh_t mesh = draw.mesh;
    // material_t material = draw.material;

    const float4 W = { kDrawWidth, kDrawHeight, 0xffff, 1.0f };
    const float kTileSize = 1.0f / kTilesPerDim;
    const float dx = 1.0f / kDrawWidth;
    const float dy = 1.0f / kDrawHeight;
    float2 tileMin = { tile.x, tile.y };
    float2 tileMax = { tile.z, tile.w };

    for (i32 iVert = 0; (iVert + 3) <= mesh.length; iVert += 3)
    {
        const float4 A = mesh.positions[iVert];
        const float4 B = mesh.positions[iVert + 1];
        const float4 C = mesh.positions[iVert + 2];

        const float2 a = { A.x, A.y };
        const float2 b = { B.x, B.y };
        const float2 c = { C.x, C.y };
        const float2 ab = f2_normalize(f2_sub(b, a));
        const float2 ac = f2_normalize(f2_sub(c, a));

        // TODO: test front face

        const float4 NA = mesh.normals[iVert];
        const float4 NB = mesh.normals[iVert + 1];
        const float4 NC = mesh.normals[iVert + 2];

        float2 lo = f2_min(a, f2_min(b, c));
        float2 hi = f2_max(a, f2_max(b, c));
        lo = f2_max(lo, tileMin);
        hi = f2_min(hi, tileMax);

        for (float y = lo.y; y < hi.y; y += dy)
        {
            for (float x = lo.x; x < hi.x; x += dx)
            {
                const float2 pt = { x, y };
                const float2 bl = { f2_dot(pt, ab), f2_dot(pt, ac) };
                if (bl.x < 0.0f || bl.y < 0.0f || (bl.x + bl.y) > 1.0f)
                {
                    continue;
                }
                float4 P = f4_blend(A, B, C, bl);

                // depth clip
                // [-1, 1] => [0, 1]
                P.z = 1.0f + 0.5f * P.z;
                if (P.z < 0.0f || P.z >= 1.0f)
                {
                    continue;
                }

                const float4 PW = f4_mul(P, W);
                const i32 iTexel = (i32)PW.y * kDrawWidth + (i32)PW.x;
                ASSERT(iTexel >= 0);
                ASSERT(iTexel < kDrawPixels);

                // depth test
                u16 Z = (u16)PW.z;
                if (Z >= frame.depth[iTexel])
                {
                    continue;
                }
                frame.depth[iTexel] = Z;

                float4 N = f4_blend(NA, NB, NC, bl);
                N.w = 0.0f;
                N = f4_normalize(N);

                // treating normal as vertex color for the moment
                u16 color = f4_rgb5a1(N);
                frame.color[iTexel] = color;
            }
        }
    }
}

static void ExecTile(rastertask_t* task, i32 iTile)
{
    const i32 iFrame = task->iFrame & kFrameMask;
    renderstate_t state;
    state.viewport = (float4) { 0.0f, 0.0f, kDrawWidth, kDrawHeight };
    state.P = (float4x4) {
        .c0 = { 1.0f, 0.0f, 0.0f, 0.0f },
        .c1 = { 0.0f, 1.0f, 0.0f, 0.0f },
        .c2 = { 0.0f, 0.0f, 1.0f, 0.0f },
        .c3 = { 0.0f, 0.0f, 0.0f, 1.0f },
    };
    state.V = (float4x4) {
        .c0 = { 1.0f, 0.0f, 0.0f, 0.0f },
        .c1 = { 0.0f, 1.0f, 0.0f, 0.0f },
        .c2 = { 0.0f, 0.0f, 1.0f, 0.0f },
        .c3 = { 0.0f, 0.0f, 0.0f, 1.0f },
    };
    state.iTile = iTile;
    state.iFrame = iFrame;

    rcmdbuf_t* cmdBuf = NULL;
    rcmdqueue_t* cmdQueue = &(ms_queues[iFrame]);

tryread:
    cmdBuf = rcmdqueue_read(cmdQueue, iTile);
    if (cmdBuf)
    {
        i32 cursor = 0;
        rcmd_t cmd;
        while (rcmdbuf_read(cmdBuf, &cursor, &cmd))
        {
            switch (cmd.type)
            {
            default:
                ASSERT(false);
                break;
            case RCmdType_Clear:
                ClearTile(state, cmd.cmd.clear);
                break;
            case RCmdType_View:
                state.V = cmd.cmd.view.V;
                state.P = cmd.cmd.view.P;
                break;
            case RCmdType_Draw:
                DrawMesh(state, cmd.cmd.draw);
                break;
            }

            goto tryread;
        }
    }
}

static void RenderTaskFn(task_t* task, i32 begin, i32 end)
{
    rastertask_t* renderTask = (rastertask_t*)task;
    for (i32 i = begin; i < end; ++i)
    {
        ExecTile(renderTask, i);
    }
}

static void DrawableTaskFn(ecs_foreach_t* task, void** rows, i32 length)
{
    ent_t* entities = (ent_t*)(rows[CompId_Entity]);
    float4* positions = (float4*)(rows[CompId_Position]);
    float4* rotations = (float4*)(rows[CompId_Rotation]);
    float4* scales = (float4*)(rows[CompId_Rotation]);
    ASSERT(entities);
    ASSERT(positions);
    ASSERT(rotations);
    ASSERT(scales);
    const float4 pos_stride = { 1.0f, 2.0f, 3.0f, 4.0f };
    const float4 quat_ident = { 0.0f, 0.0f, 0.0f, 1.0f };
    const float4 scale_ident = { 1.0f, 1.0f, 1.0f, 1.0f };

    for (i32 i = 0; i < length; ++i)
    {
        positions[i] = f4_mul(f4_s(i * kPi), pos_stride);
    }

    for (i32 i = 0; i < length; ++i)
    {
        rotations[i] = quat_ident;
    }

    for (i32 i = 0; i < length; ++i)
    {
        scales[i] = scale_ident;
    }
}

static void CreateEntities(task_t* task, i32 begin, i32 end)
{
    compflag_t all = compflag_create(3, CompId_Position, CompId_Rotation, CompId_Scale);
    compflag_t some = compflag_create(1, CompId_Position);
    for (i32 i = begin; i < end; ++i)
    {
        ecs_create(rand_int() & 1 ? all : some);
    }
}

static void* ImGuiAllocFn(usize sz, void* userData) { return perm_malloc((i32)sz); }
static void ImGuiFreeFn(void* ptr, void* userData) { pim_free(ptr); }

i32 screen_width(void) { return ms_width; }
i32 screen_height(void) { return ms_height; }

void render_sys_init(void)
{
    ms_iFrame = 0;
    sg_setup(&(sg_desc)
    {
        .mtl_device = sapp_metal_get_device(),
        .mtl_drawable_cb = sapp_metal_get_drawable,
        .mtl_renderpass_descriptor_cb = sapp_metal_get_renderpass_descriptor,
        .d3d11_device = sapp_d3d11_get_device(),
        .d3d11_device_context = sapp_d3d11_get_device_context(),
        .d3d11_render_target_view_cb = sapp_d3d11_get_render_target_view,
        .d3d11_depth_stencil_view_cb = sapp_d3d11_get_depth_stencil_view,
    });
    ms_features = sg_query_features();
    ms_limits = sg_query_limits();
    ms_backend = sg_query_backend();

    sgl_setup(&(sgl_desc_t) { 0 });
    igSetAllocatorFunctions(ImGuiAllocFn, ImGuiFreeFn, NULL);
    simgui_setup(&(simgui_desc_t) { 0 });

    ms_width = sapp_width();
    ms_height = sapp_height();

    for (i32 i = 0; i < kNumFrames; ++i)
    {
        ms_images[i] = sg_make_image(&(sg_image_desc)
        {
            .type = SG_IMAGETYPE_2D,
            .pixel_format = SG_PIXELFORMAT_RGB5A1,
            .width = kDrawWidth,
            .height = kDrawHeight,
            .usage = SG_USAGE_STREAM,
            .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
            .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
        });
        framebuf_create(ms_buffers + i, kDrawWidth, kDrawHeight);
        rcmdqueue_create(ms_queues + i);
        ms_rastasks[i] = (rastertask_t)
        {
            .iFrame = i,
        };
        ms_cmdtasks[i] = (cmdtask_t)
        {
            0
        };
    }

    CreateEntities(NULL, 0, 1 << 20);
}

void render_sys_update(void)
{
    const i32 iFrame = ms_iFrame & kFrameMask;

    ms_width = sapp_width();
    ms_height = sapp_height();
    simgui_new_frame(ms_width, ms_height, time_dtf());

    compflag_t all = compflag_create(3, CompId_Position, CompId_Rotation, CompId_Scale);
    compflag_t none = compflag_create(0);
    ecs_foreach(&(ms_cmdtasks[iFrame].task), all, none, DrawableTaskFn);

    rcmdbuf_t* cmdbuf = rcmdbuf_create();
    rcmd_clear(cmdbuf, 0x0000, 0xffff);

    float4x4 M = { 0 };
    mesh_t mesh = { 0 };
    mesh.length = 3;
    mesh.positions = pim_malloc(EAlloc_Temp, sizeof(float4) * 3);
    mesh.normals = pim_malloc(EAlloc_Temp, sizeof(float4) * 3);
    // GL defaults to counter-clock-wise
    mesh.positions[0] = (float4) { -1.0f, 1.0f, 0.5f, 1.0f };
    mesh.positions[1] = (float4) { -1.0f, -1.0f, 0.5f, 1.0f };
    mesh.positions[2] = (float4) { 1.0f, 1.0f, 0.5f, 1.0f };
    mesh.normals[0] = (float4) { 1.0f, 0.0f, 0.0f, 0.0f };
    mesh.normals[1] = (float4) { 0.0f, 1.0f, 0.0f, 0.0f };
    mesh.normals[2] = (float4) { 0.0f, 0.0f, 1.0f, 0.0f };
    material_t material = { 0 };
    rcmd_draw(cmdbuf, M, mesh, material);

    rcmdqueue_submit(ms_queues + iFrame, cmdbuf);
}

void render_sys_shutdown(void)
{
    task_sys_schedule();

    for (i32 i = 0; i < kNumFrames; ++i)
    {
        task_await((task_t*)(ms_cmdtasks + i));
        task_await((task_t*)(ms_rastasks + i));
        sg_destroy_image(ms_images[i]);
        framebuf_destroy(ms_buffers + i);
        rcmdqueue_destroy(ms_queues + i);
    }

    simgui_shutdown();
    sgl_shutdown();
    sg_shutdown();
}

void render_sys_frameend(void)
{
    const i32 iPrev = (ms_iFrame - 1) & kFrameMask;
    const i32 iCurrent = (ms_iFrame + 0) & kFrameMask;
    {
        rastertask_t* task = ms_rastasks + iCurrent;
        task_await((task_t*)task);

        framebuf_t buffer = ms_buffers[iCurrent];
        sg_update_image(ms_images[iCurrent], &(sg_image_content){
            .subimage[0][0].ptr = buffer.color,
            .subimage[0][0].size = framebuf_color_bytes(buffer),
        });

        task->iFrame = iCurrent;
        task_submit((task_t*)task, RenderTaskFn, kTileCount);
    }
    {
        sg_begin_default_pass(&(sg_pass_action) { 0 }, ms_width, ms_height);
        sgl_viewport(0, 0, ms_width, ms_height, ms_features.origin_top_left);
        sgl_enable_texture();
        sgl_matrix_mode_texture();
        sgl_load_identity();
        sgl_texture(ms_images[iPrev]);
        sgl_begin_triangles();
        {
            sgl_v2f_t2f(-1.0f, -1.0f, 0.0f, 0.0f); // TL
            sgl_v2f_t2f(-1.0f, 1.0f, 0.0f, 1.0f); // BL
            sgl_v2f_t2f(1.0f, -1.0f, 1.0f, 0.0f); // TR

            sgl_v2f_t2f(1.0f, -1.0f, 1.0f, 0.0f); // TR
            sgl_v2f_t2f(-1.0f, 1.0f, 0.0f, 1.0f); // BL
            sgl_v2f_t2f(1.0f, 1.0f, 1.0f, 1.0f); // BR
        }
        sgl_end();
        sgl_draw();
        simgui_render();
        sg_end_pass();
        sg_commit();
        ++ms_iFrame;
    }
}

i32 render_sys_onevent(const struct sapp_event* evt)
{
    return simgui_handle_event(evt);
}
