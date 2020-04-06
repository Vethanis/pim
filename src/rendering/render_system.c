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
#include "math/float4_funcs.h"
#include "math/float2_funcs.h"
#include "math/float4x4.h"
#include "common/random.h"
#include "containers/idset.h"
#include "containers/ptrqueue.h"
#include <string.h>

typedef struct rendertask_s
{
    task_t task;
    framebuf_t buffer;
    ptrqueue_t* pCmdQueue;
} rendertask_t;

math_inline float4 VEC_CALL f4_rand(void)
{
    return f4_v(rand_float(), rand_float(), rand_float(), rand_float());
}

math_inline float2 VEC_CALL f2_rand(void)
{
    return f2_v(rand_float(), rand_float());
}

math_inline u16 f4_rgb5a1(float4 v)
{
    u16 r = (u16)(v.x * 31.0f) & 31;
    u16 g = (u16)(v.y * 31.0f) & 31;
    u16 b = (u16)(v.z * 31.0f) & 31;
    u16 c = (r << 11) | (g << 6) | (b << 1) | 1;
    return c;
}

static float2 GetTile(i32 i)
{
    i32 x = (i & 7) * kTileWidth;
    i32 y = (i >> 3) * kTileHeight;
    return f2_v((float)x, (float)y);
}

static void DrawTile(rendertask_t* task, i32 iTile)
{
    float2 tile = GetTile(iTile);
    framebuf_t buf = task->buffer;

    const float2 dv = { 1.0f / kDrawWidth, 1.0f / kDrawHeight };
    const float kDither = 1.0f / (1 << 5);
    for (float ty = 0.0f; ty < kTileHeight; ++ty)
    {
        for (float tx = 0.0f; tx < kTileWidth; ++tx)
        {
            float2 uv = f2_add(tile, f2_v(tx, ty));
            float2 color = f2_lerp(f2_mul(uv, dv), f2_rand(), kDither);

            const i32 i = (i32)(kDrawWidth * uv.y + uv.x);
            buf.color[i] = f4_rgb5a1(f4_v(color.x, color.y, 0.0f, 1.0f));
        }
    }
}

static void RenderTaskFn(task_t* task, i32 begin, i32 end)
{
    rendertask_t* renderTask = (rendertask_t*)task;
    for (i32 i = begin; i < end; ++i)
    {
        DrawTile(renderTask, i);
    }
}

typedef struct drawabletask_s
{
    ecs_foreach_t task;
} drawabletask_t;

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

static i32 ms_width;
static i32 ms_height;
static sg_features ms_features;
static sg_limits ms_limits;
static sg_backend ms_backend;
static i32 ms_iFrame;
static sg_image ms_images[kNumFrames];
static rendertask_t ms_tasks[kNumFrames];
static framebuf_t ms_buffers[kNumFrames];
static drawabletask_t ms_drawableTask;

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

    const sg_image_desc img =
    {
        .type = SG_IMAGETYPE_2D,
        .pixel_format = SG_PIXELFORMAT_RGB5A1,
        .width = kDrawWidth,
        .height = kDrawHeight,
        .usage = SG_USAGE_STREAM,
        .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
        .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
    };
    for (i32 i = 0; i < kNumFrames; ++i)
    {
        ms_images[i] = sg_make_image(&img);
    }

    sgl_setup(&(sgl_desc_t) { 0 });
    igSetAllocatorFunctions(ImGuiAllocFn, ImGuiFreeFn, NULL);
    simgui_setup(&(simgui_desc_t) { 0 });

    ms_width = sapp_width();
    ms_height = sapp_height();

    for (i32 i = 0; i < kNumFrames; ++i)
    {
        framebuf_create(ms_buffers + i, kDrawWidth, kDrawHeight);
    }

    CreateEntities(NULL, 0, 1 << 20);
}

void render_sys_update(void)
{
    ms_width = sapp_width();
    ms_height = sapp_height();
    simgui_new_frame(ms_width, ms_height, time_dtf());

    compflag_t all = compflag_create(3, CompId_Position, CompId_Rotation, CompId_Scale);
    compflag_t none = compflag_create(0);
    ecs_foreach(&ms_drawableTask.task, all, none, DrawableTaskFn);
}

void render_sys_shutdown(void)
{
    task_sys_schedule();

    for (i32 i = 0; i < kNumFrames; ++i)
    {
        task_await(&(ms_tasks[i].task));
        sg_destroy_image(ms_images[i]);
        framebuf_destroy(ms_buffers + i);
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
        task_await(&(ms_tasks[iCurrent].task));
        framebuf_t buffer = ms_buffers[iCurrent];
        sg_update_image(ms_images[iCurrent], &(sg_image_content){
            .subimage[0][0].ptr = buffer.color,
            .subimage[0][0].size = framebuf_color_bytes(buffer),
        });
        framebuf_clear(buffer, 0, 0xffff);
        ms_tasks[iCurrent].buffer = buffer;
        task_submit(&(ms_tasks[iCurrent].task), RenderTaskFn, kTileCount);
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
