#include "rendering/render_system.h"

#include <sokol/sokol_gfx.h>
#include <sokol/sokol_app.h>
#include <sokol/util/sokol_imgui.h>
#include <sokol/util/sokol_gl.h>
#include "ui/imgui.h"

#include "common/time.h"
#include "components/ecs.h"
#include "rendering/screen.h"
#include "rendering/framebuffer.h"
#include "math/vec_funcs.h"
#include "common/random.h"
#include "containers/idset.h"

#include <string.h>

namespace Screen
{
    static i32 ms_width;
    static i32 ms_height;

    i32 Width() { return ms_width; }
    i32 Height() { return ms_height; }

    static void Update()
    {
        Screen::ms_width = sapp_width();
        Screen::ms_height = sapp_height();
    }
};

static constexpr i32 kNumFrames = 4;
static constexpr i32 kFrameMask = kNumFrames - 1;
static constexpr i32 kDrawWidth = 320;
static constexpr i32 kDrawHeight = 240;
static constexpr i32 kDrawPixels = kDrawWidth * kDrawHeight;
static constexpr i32 kTileCount = 8 * 8;
static constexpr i32 kTileWidth = kDrawWidth / 8;
static constexpr i32 kTileHeight = kDrawHeight / 8;
static constexpr i32 kTilePixels = kTileWidth * kTileHeight;

static int2 GetTile(i32 i)
{
    i32 x = i & 7;
    i32 y = i >> 3;
    return int2(x, y);
}

static void DrawTile(framebuf_t buf, i32 x0, i32 y0)
{
    x0 *= kTileWidth;
    y0 *= kTileHeight;
    const float dx = 1.0f / kDrawWidth;
    const float dy = 1.0f / kDrawHeight;
    const float kDither = 1.0f / (1 << 5);
    for (i32 ty = 0; ty < kTileHeight; ++ty)
    {
        for (i32 tx = 0; tx < kTileWidth; ++tx)
        {
            const i32 x = x0 + tx;
            const i32 y = y0 + ty;
            float color[4] = { x * dx, y * dy, 0.0f, 1.0f };
            f4_dither(color, kDither);
            framebuf_wcolor(buf, x, y, color);
        }
    }
}

typedef struct mesh_s
{
    float* positions;
    float* uvs;
    int32_t length;
} mesh_t;

typedef struct drawcmd_s
{
    float matrix[16];
    mesh_t mesh;
} drawcmd_t;

typedef struct cmdbuf_s
{
    int32_t* ids;
    void* cmds;
    int32_t length;
} cmdbuf_t;

typedef struct camera_s
{
    float V[16];
    float P[16];
} camera_t;

typedef struct rendertask_s
{
    task_t task;
    framebuf_t buffer;
    camera_t camera;
    cmdbuf_t cmds;
} rendertask_t;

static void RenderTaskFn(task_t* task, int32_t begin, int32_t end)
{
    rendertask_t* renderTask = (rendertask_t*)task;
    for (int32_t i = begin; i < end; ++i)
    {
        int2 tile = GetTile(i);
        DrawTile(renderTask->buffer, tile.x, tile.y);
    }
}

typedef struct drawabletask_s
{
    ecs_foreach_t task;
} drawabletask_t;

static void DrawableTaskFn(ecs_foreach_t* task, void** rows, int32_t length)
{
    ent_t* entities = (ent_t*)(rows[CompId_Entity]);
    float4* positions = (float4*)(rows[CompId_Position]);
    quaternion* rotations = (quaternion*)(rows[CompId_Rotation]);
    float4* scales = (float4*)(rows[CompId_Rotation]);
    ASSERT(entities);
    ASSERT(positions);
    ASSERT(rotations);
    ASSERT(scales);
    for (int32_t i = 0; i < length; ++i)
    {
        positions[i] = float4(i * 1.0f, i * 2.0f, i * 3.0f, i * 4.0f);
        rotations[i] = { float4(0.0f, 0.0f, 0.0f, 1.0f) };
        scales[i] = float4(1.0f, 1.0f, 1.0f, 0.0f);
    }
}

static sg_features ms_features;
static sg_limits ms_limits;
static sg_backend ms_backend;
static i32 ms_iFrame;
static sg_image ms_images[kNumFrames];
static rendertask_t ms_tasks[kNumFrames];
static framebuf_t ms_buffers[kNumFrames];
static drawabletask_t ms_drawableTask;

static void* ImGuiAllocFn(size_t sz, void*) { return perm_malloc((int32_t)sz); }
static void ImGuiFreeFn(void* ptr, void*) { pim_free(ptr); }

extern "C" void render_sys_init(void)
{
    ms_iFrame = 0;
    {
        sg_desc desc = {};
        desc.mtl_device = sapp_metal_get_device();
        desc.mtl_drawable_cb = sapp_metal_get_drawable;
        desc.mtl_renderpass_descriptor_cb = sapp_metal_get_renderpass_descriptor;
        desc.d3d11_device = sapp_d3d11_get_device();
        desc.d3d11_device_context = sapp_d3d11_get_device_context();
        desc.d3d11_render_target_view_cb = sapp_d3d11_get_render_target_view;
        desc.d3d11_depth_stencil_view_cb = sapp_d3d11_get_depth_stencil_view;
        sg_setup(&desc);
        ms_features = sg_query_features();
        ms_limits = sg_query_limits();
        ms_backend = sg_query_backend();

        sg_image_desc img = {};
        img.type = SG_IMAGETYPE_2D;
        img.pixel_format = SG_PIXELFORMAT_RGB5A1;
        img.width = kDrawWidth;
        img.height = kDrawHeight;
        img.usage = SG_USAGE_STREAM;
        img.wrap_u = SG_WRAP_CLAMP_TO_EDGE;
        img.wrap_v = SG_WRAP_CLAMP_TO_EDGE;

        for (i32 i = 0; i < kNumFrames; ++i)
        {
            ms_images[i] = sg_make_image(&img);
        }
    }
    {
        sgl_desc_t desc = {};
        sgl_setup(&desc);
    }
    {
        ImGui::SetAllocatorFunctions(ImGuiAllocFn, ImGuiFreeFn);
        simgui_desc_t desc = {};
        simgui_setup(&desc);
    }
    Screen::Update();

    for (int32_t i = 0; i < kNumFrames; ++i)
    {
        framebuf_create(ms_buffers + i, kDrawWidth, kDrawHeight);
    }

    compflag_t all = compflag_create(3, CompId_Position, CompId_Rotation, CompId_Scale);
    compflag_t some = compflag_create(1, CompId_Position);
    for (int32_t i = 0; i < (1 << 20); ++i)
    {
        ecs_create((rand_int() & 1) ? all : some);
    }
}

extern "C" void render_sys_update(void)
{
    Screen::Update();
    simgui_new_frame(Screen::Width(), Screen::Height(), time_dtf());

    compflag_t all = compflag_create(3, CompId_Position, CompId_Rotation, CompId_Scale);
    compflag_t none = compflag_create(0);
    ecs_foreach(&ms_drawableTask.task, all, none, DrawableTaskFn);
}

extern "C" void render_sys_shutdown(void)
{
    for (i32 i = 0; i < kNumFrames; ++i)
    {
        task_await(&(ms_tasks[i].task));
        sg_destroy_image(ms_images[i]);
        ms_images[i] = {};
        framebuf_destroy(ms_buffers + i);
    }

    simgui_shutdown();
    sgl_shutdown();
    sg_shutdown();
}

extern "C" void render_sys_frameend(void)
{
    const i32 iPrev = (ms_iFrame - 1) & kFrameMask;
    const i32 iCurrent = (ms_iFrame + 0) & kFrameMask;
    {
        task_await(&(ms_tasks[iCurrent].task));
        framebuf_t buffer = ms_buffers[iCurrent];
        sg_image_content content = {};
        content.subimage[0][0] =
        {
            buffer.color,
            framebuf_color_bytes(buffer),
        };
        sg_update_image(ms_images[iCurrent], &content);
        float color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        framebuf_clear(buffer, color, 1.0f);
        ms_tasks[iCurrent].buffer = buffer;
        task_submit(&(ms_tasks[iCurrent].task), RenderTaskFn, kTileCount);
    }
    {
        sg_pass_action clear = {};
        sg_begin_default_pass(&clear, Screen::Width(), Screen::Height());
        sgl_viewport(0, 0, Screen::Width(), Screen::Height(), ms_features.origin_top_left);
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

extern "C" int32_t render_sys_onevent(const struct sapp_event* evt)
{
    return simgui_handle_event(evt);
}
