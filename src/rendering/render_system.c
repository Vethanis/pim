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
#include "math/types.h"
#include "math/float4_funcs.h"
#include "math/float3_funcs.h"
#include "math/float2_funcs.h"
#include "math/float4x4_funcs.h"
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
    mesh_t vstream;
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

static const int2 kTileSize = { kTileWidth, kTileHeight };
static const float2 kRcpScreen = { 1.0f / kDrawWidth, 1.0f / kDrawHeight };

pim_inline float4 VEC_CALL f4_rand(prng_t* rng)
{
    return f4_v(prng_f32(rng), prng_f32(rng), prng_f32(rng), prng_f32(rng));
}

pim_inline float2 VEC_CALL f2_rand(prng_t* rng)
{
    return f2_v(prng_f32(rng), prng_f32(rng));
}

pim_inline float4 VEC_CALL f4_dither(prng_t* rng, float4 x)
{
    const float kDither = 1.0f / (1 << 5);
    return f4_lerp(x, f4_rand(rng), kDither);
}

pim_inline float2 VEC_CALL i2_f2(int2 v)
{
    return f2_iv(v.x, v.y);
}

pim_inline float4 VEC_CALL f4_blend(float4 a, float4 b, float4 c, float w, float u, float v)
{
    float4 p = f4_mulvs(a, w);
    p = f4_add(p, f4_mulvs(b, u));
    p = f4_add(p, f4_mulvs(c, v));
    return p;
}

pim_inline float4 VEC_CALL f4_unorm(float4 s)
{
    return f4_addvs(f4_mulvs(s, 0.5f), 0.5f);
}

pim_inline float4 VEC_CALL f4_snorm(float4 u)
{
    return f4_subvs(f4_mulvs(u, 2.0f), 1.0f);
}

pim_inline float3 VEC_CALL f4_f3(float4 v)
{
    return (float3) { v.x, v.y, v.z };
}

pim_inline float2 VEC_CALL f4_f2(float4 v)
{
    return (float2) { v.x, v.y };
}

pim_inline u16 VEC_CALL f4_rgb5a1(float4 v)
{
    const float4 kScale = { 31.0f, 31.0f, 31.0f, 31.0f };
    v = f4_mul(v, kScale);
    u16 r = (u16)v.x;
    u16 g = (u16)v.y;
    u16 b = (u16)v.z;
    u16 c = (r << 11) | (g << 6) | (b << 1) | 1;
    return c;
}

pim_inline float4 VEC_CALL ray_tri_isect(
    float3 O, float3 D,
    float3 A, float3 B, float3 C)
{
    float4 result = { 0.0f, 0.0f, 0.0f, 0.0f };
    float3 BA = f3_sub(B, A);
    float3 CA = f3_sub(C, A);
    float3 P = f3_cross(D, CA);
    float det = f3_dot(BA, P);
    if (det > 0.001f)
    {
        float rcpDet = 1.0f / det;
        float3 T = f3_sub(O, A);
        float3 Q = f3_cross(T, BA);

        float t = f3_dot(CA, Q) * rcpDet;
        float u = f3_dot(T, P) * rcpDet;
        float v = f3_dot(D, Q) * rcpDet;

        float tU = (u >= 0.0f) ? 1.0f : 0.0f;
        float tV = (v >= 0.0f) ? 1.0f : 0.0f;
        float tUV = ((u + v) <= 1.0f) ? 1.0f : 0.0f;
        float tT = (t > 0.0f) ? 1.0f : 0.0f;

        result.x = u;
        result.y = v;
        result.z = t;
        result.w = (tU + tV + tUV + tT) * 0.25f;
    }
    return result;
}

pim_inline float VEC_CALL sdPlane2D(float2 n, float d, float2 pt)
{
    return f2_dot(n, pt) - d;
}

pim_inline float VEC_CALL sdPlaneBox2D(float2 n, float d, float2 center, float2 extents)
{
    return sdPlane2D(n, d, center) - f32_abs(f2_dot(n, extents));
}

float sdTriangle2D(float2 a, float2 b, float2 c, float2 p)
{
    float2 mid = f2_mulvs(f2_add(a, f2_add(b, c)), 0.33333333f);
    float2 abN = f2_normalize(f2_sub(f2_lerp(a, b, 0.5f), mid));
    float2 acN = f2_normalize(f2_sub(f2_lerp(a, c, 0.5f), mid));
    float2 bcN = f2_normalize(f2_sub(f2_lerp(b, c, 0.5f), mid));
    float dab = sdPlane2D(abN, f2_dot(abN, a), p);
    float dac = sdPlane2D(acN, f2_dot(acN, a), p);
    float dbc = sdPlane2D(bcN, f2_dot(bcN, b), p);
    return f32_max(dab, f32_max(dac, dbc));
}

float sdTriangleBox2D(float2 a, float2 b, float2 c, float2 center, float2 extents)
{
    float2 mid = f2_mulvs(f2_add(a, f2_add(b, c)), 0.33333333f);
    float2 nab = f2_normalize(f2_sub(f2_lerp(a, b, 0.5f), mid));
    float2 nac = f2_normalize(f2_sub(f2_lerp(a, c, 0.5f), mid));
    float2 nbc = f2_normalize(f2_sub(f2_lerp(b, c, 0.5f), mid));
    float dab = sdPlaneBox2D(nab, f2_dot(nab, a), center, extents);
    float dac = sdPlaneBox2D(nac, f2_dot(nac, a), center, extents);
    float dbc = sdPlaneBox2D(nbc, f2_dot(nbc, b), center, extents);
    return f32_max(dab, f32_max(dac, dbc));
}

pim_inline int2 VEC_CALL GetScreenTile(i32 i)
{
    i32 x = (i % kTilesPerDim);
    i32 y = (i / kTilesPerDim);
    return (int2) { x * kTileWidth, y * kTileHeight };
}

pim_inline float4 VEC_CALL GetUnormTile(i32 i)
{
    int2 tile = GetScreenTile(i);
    int2 iHi = { tile.x + kTileWidth, tile.y + kTileHeight };
    float2 lo = f2_mul(i2_f2(tile), kRcpScreen);
    float2 hi = f2_mul(i2_f2(iHi), kRcpScreen);
    return (float4) { lo.x, lo.y, hi.x, hi.y };
}

static void VEC_CALL ClearTile(renderstate_t state, rcmd_clear_t clear)
{
    framebuf_t frame = ms_buffers[state.iFrame];
    const int2 tile = GetScreenTile(state.iTile);
    for (i32 ty = 0; ty < kTileHeight; ++ty)
    {
        for (i32 tx = 0; tx < kTileWidth; ++tx)
        {
            i32 x = tile.x + tx;
            i32 y = tile.y + ty;
            i32 i = kDrawWidth * y + x;
            frame.color[i] = clear.color;
        }
    }
    for (i32 ty = 0; ty < kTileHeight; ++ty)
    {
        for (i32 tx = 0; tx < kTileWidth; ++tx)
        {
            i32 x = tile.x + tx;
            i32 y = tile.y + ty;
            i32 i = kDrawWidth * y + x;
            frame.depth[i] = clear.depth;
        }
    }
}

pim_inline bool VEC_CALL f3_cliptest(float3 x, float lo, float hi)
{
    return f3_all(f3_ltvs(x, lo)) || f3_all(f3_gtvs(x, hi));
}

// pineda edge function (flipped for right handedness)
// returns > 0 when on right side of A -> B
// returns 0 when exactly on A -> B
// returns < 0 when on left side of A -> B
// returns 2x the area of the triangle formed by A -> B -> P
// (also is its determinant aka cross product in two dimensions)
pim_inline float VEC_CALL edge_func(float2 A, float2 B, float2 P)
{
    float2 AP = f2_sub(P, A);
    float2 AB = f2_sub(B, A);
    // the determinant of
    // [AP.x, AP.y]
    // [AB.x, AB.y]
    float e = AP.y * AB.x - AP.x * AB.y;
    return e;
}

pim_inline u16 VEC_CALL f4_color(prng_t* rng, float4 x)
{
    return f4_rgb5a1(f4_dither(rng, x));
}

static void VEC_CALL DrawMesh(renderstate_t state, rcmd_draw_t draw)
{
    framebuf_t frame = ms_buffers[state.iFrame];
    // int2 tile = GetScreenTile(state.iTile);
    // float4x4 M = draw.M;
    const mesh_t mesh = draw.mesh;
    // material_t material = draw.material;

    const float4 tileRange = GetUnormTile(state.iTile);
    const float2 tileCenter = {
        0.5f * (tileRange.x + tileRange.z),
        0.5f * (tileRange.y + tileRange.w) };
    const float2 tileExtents = {
        tileRange.z - tileCenter.x,
        tileRange.w - tileCenter.y };
    const float tileLen = f2_length(tileExtents);
    const float dz = 0xffff;
    prng_t rng;
    prng_create(&rng);

    for (i32 iVert = 0; (iVert + 3) <= mesh.length; iVert += 3)
    {
        // local space
        float4 A = mesh.positions[iVert];
        float4 B = mesh.positions[iVert + 1];
        float4 C = mesh.positions[iVert + 2];

        // projection
        A = f4x4_mul_pt(state.P, A);
        B = f4x4_mul_pt(state.P, B);
        C = f4x4_mul_pt(state.P, C);
        // perspective divide
        A = f4_divvs(A, A.w);
        B = f4_divvs(B, B.w);
        C = f4_divvs(C, C.w);
        // [-1, 1] => [0, 1]
        A = f4_unorm(A);
        B = f4_unorm(B);
        C = f4_unorm(C);

        const float2 a = f4_f2(A);
        const float2 b = f4_f2(B);
        const float2 c = f4_f2(C);
        const float area2 = edge_func(a, b, c);

        // front face test
        if (area2 < f16_eps)
        {
            continue;
        }

        // test if all 3 verts are inside clip space and tile range
        if (f3_cliptest(f3_v(A.z, B.z, C.z), 0.0f, 1.0f))
        {
            continue;
        }

        // tile cull
        //if (sdTriangleBox2D(a, b, c, tileCenter, tileExtents) > 0.01f)
        //{
        //    continue;
        //}

        // interpolators
        const float4 NA = mesh.normals[iVert];
        const float4 NB = mesh.normals[iVert + 1];
        const float4 NC = mesh.normals[iVert + 2];

        const float yMin = f32_max(tileRange.y, f3_hmin(f3_v(a.y, b.y, c.y)));
        const float yMax = f32_min(tileRange.w, f3_hmax(f3_v(a.y, b.y, c.y)));
        const float xMin = f32_max(tileRange.x, f3_hmin(f3_v(a.x, b.x, c.x)));
        const float xMax = f32_min(tileRange.z, f3_hmax(f3_v(a.x, b.x, c.x)));
        const float dx = 1.0f / kDrawWidth;
        const float dy = 1.0f / kDrawHeight;
        const float rcpArea2 = 1.0f / area2;

        for (float y = yMin; y <= yMax; y += dy)
        {
            for (float x = xMin; x <= xMax; x += dx)
            {
                const float2 p = f2_v(x, y);
                const float w = edge_func(b, c, p) * rcpArea2;
                const float u = edge_func(c, a, p) * rcpArea2;
                const float v = edge_func(a, b, p) * rcpArea2;

                if (w < 0.0f || u < 0.0f || v < 0.0f)
                {
                    continue;
                }

                float4 P = f4_blend(A, B, C, w, u, v);
                // depth clip
                if (P.z > 1.0f || P.z < 0.0f)
                {
                    continue;
                }

                const float2 pSc = f2_floor(f2_v(p.x * kDrawWidth, p.y * kDrawHeight));
                const i32 iTexel = (i32)pSc.x + kDrawWidth * (i32)pSc.y;
                ASSERT(iTexel >= 0);
                ASSERT(iTexel < kDrawPixels);

                // depth test
                const u16 Z = (u16)(P.z * dz);
                if (Z >= frame.depth[iTexel])
                {
                    continue;
                }
                frame.depth[iTexel] = Z;

                // blend interpolators
                float4 N = f4_blend(NA, NB, NC, w, u, v);

                // treating normal as vertex color for the moment
                frame.color[iTexel] = f4_color(&rng, N);
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

    while((cmdBuf = rcmdqueue_read(cmdQueue, iTile)))
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
    ent_t* __restrict entities = (ent_t*)(rows[CompId_Entity]);
    float4* __restrict positions = (float4*)(rows[CompId_Position]);
    float4* __restrict rotations = (float4*)(rows[CompId_Rotation]);
    float4* __restrict scales = (float4*)(rows[CompId_Rotation]);
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
    prng_t rng;
    prng_create(&rng);
    compflag_t all = compflag_create(3, CompId_Position, CompId_Rotation, CompId_Scale);
    compflag_t some = compflag_create(1, CompId_Position);
    for (i32 i = begin; i < end; ++i)
    {
        ecs_create(prng_i32(&rng) & 1 ? all : some);
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

    float aspect = (float)ms_width / (float)ms_height;
    const float fovy = f32_radians(90.0f);
    rcmd_view(cmdbuf, f4x4_id, f4x4_perspective(fovy, aspect, 0.05f, 200.0f));

    float4x4 M = { 0 };
    mesh_t mesh = { 0 };
    mesh.length = 3;
    mesh.positions = pim_malloc(EAlloc_Temp, sizeof(float4) * 3);
    mesh.normals = pim_malloc(EAlloc_Temp, sizeof(float4) * 3);
    // GL defaults to counter-clock-wise
    mesh.positions[0] = (float4) { -1.0f, -1.0f, -2.0f, 1.0f };
    mesh.positions[1] = (float4) { 1.0f, -1.0f, -2.0f, 1.0f };
    mesh.positions[2] = (float4) { 0.0f, 1.0f, -2.0f, 1.0f };
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
