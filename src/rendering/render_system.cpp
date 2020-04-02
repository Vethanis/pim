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
#include "math/vec.h"
#include "common/random.h"
#include "containers/idset.h"

#include <string.h>

namespace Screen
{
    static int32_t ms_width;
    static int32_t ms_height;

    int32_t Width() { return ms_width; }
    int32_t Height() { return ms_height; }

    static void Update()
    {
        Screen::ms_width = sapp_width();
        Screen::ms_height = sapp_height();
    }
};

static constexpr int32_t kNumFrames = 4;
static constexpr int32_t kFrameMask = kNumFrames - 1;
static constexpr int32_t kDrawWidth = 320;
static constexpr int32_t kDrawHeight = 240;
static constexpr int32_t kDrawPixels = kDrawWidth * kDrawHeight;
static constexpr int32_t kTileCount = 8 * 8;
static constexpr int32_t kTileWidth = kDrawWidth / 8;
static constexpr int32_t kTileHeight = kDrawHeight / 8;
static constexpr int32_t kTilePixels = kTileWidth * kTileHeight;

static void GetTile(int32_t i, int32_t* x, int32_t* y)
{
    *x = (i & 7) * kTileWidth;
    *y = (i >> 3) * kTileHeight;
}

static void DrawTile(framebuf_t buf, int32_t iTile)
{
    int32_t tx0, ty0;
    GetTile(iTile, &tx0, &ty0);
    const float dx = 1.0f / kDrawWidth;
    const float dy = 1.0f / kDrawHeight;
    const float kDither = 1.0f / (1 << 5);
    for (int32_t ty = 0; ty < kTileHeight; ++ty)
    {
        for (int32_t tx = 0; tx < kTileWidth; ++tx)
        {
            const int32_t x = tx0 + tx;
            const int32_t y = ty0 + ty;
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
        DrawTile(renderTask->buffer, i);
    }
}

typedef struct drawabletask_s
{
    ecs_foreach_t task;
} drawabletask_t;

static void DrawableTaskFn(ecs_foreach_t* task, void** rows, int32_t length)
{
    ent_t* entities = (ent_t*)(rows[CompId_Entity]);
    vec_t* positions = (vec_t*)(rows[CompId_Position]);
    vec_t* rotations = (vec_t*)(rows[CompId_Rotation]);
    vec_t* scales = (vec_t*)(rows[CompId_Rotation]);
    ASSERT(entities);
    ASSERT(positions);
    ASSERT(rotations);
    ASSERT(scales);
    const vec_t pos_stride = vec_set(1.0f, 2.0f, 3.0f, 4.0f);
    const vec_t quat_ident = vec_set(0.0f, 0.0f, 0.0f, 1.0f);
    const vec_t scale_ident = vec_one();
    for (int32_t i = 0; i < length; ++i)
    {
        positions[i] = vec_mul(vec_set1((float)i), pos_stride);
        rotations[i] = quat_ident;
        scales[i] = scale_ident;
    }
}

static sg_features ms_features;
static sg_limits ms_limits;
static sg_backend ms_backend;
static int32_t ms_iFrame;
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

        for (int32_t i = 0; i < kNumFrames; ++i)
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
    for (int32_t i = 0; i < kNumFrames; ++i)
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
    const int32_t iPrev = (ms_iFrame - 1) & kFrameMask;
    const int32_t iCurrent = (ms_iFrame + 0) & kFrameMask;
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
