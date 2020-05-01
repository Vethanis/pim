#include "rendering/render_system.h"

#include "allocator/allocator.h"
#include "common/time.h"
#include "common/cvar.h"
#include "components/ecs.h"
#include "rendering/framebuffer.h"
#include "rendering/constants.h"
#include "rendering/rcmd.h"
#include "rendering/camera.h"
#include "rendering/window.h"
#include "rendering/screenblit.h"
#include "math/types.h"
#include "math/int2_funcs.h"
#include "math/float4_funcs.h"
#include "math/float3_funcs.h"
#include "math/float2_funcs.h"
#include "math/float4x4_funcs.h"
#include "math/sdf.h"
#include "math/frustum.h"
#include "ui/cimgui.h"
#include "common/profiler.h"

// ----------------------------------------------------------------------------

typedef struct renderstate_s
{
    float4 viewport;
    camera_t camera;
    prng_t rng;
    i32 iTile;
} renderstate_t;

// ----------------------------------------------------------------------------

static const float2 kTileExtents =
{
    (0.5f * kTileWidth) / kDrawWidth,
    (0.5f * kTileHeight) / kDrawHeight
};

static const float2 kTileSize =
{
    (float)kTileWidth / kDrawWidth,
    (float)kTileHeight / kDrawHeight
};

static const compflag_t kLocalToWorldFilter =
{
    (1 << CompId_LocalToWorld)
};

static const compflag_t kDrawableFilter =
{
    (1 << CompId_LocalToWorld) |
    (1 << CompId_Drawable) |
    (1 << CompId_Bounds)
};

// ----------------------------------------------------------------------------

pim_inline int2 VEC_CALL GetTile(i32 i);
static void VEC_CALL DrawMesh(renderstate_t state, rcmd_draw_t draw);
static void VEC_CALL ClearTile(renderstate_t state, rcmd_clear_t clear);
static void ExecTile(i32 iTile);
static void RasterizeTaskFn(task_t* task, i32 begin, i32 end);
static void DrawableTaskFn(ecs_foreach_t* task, void** rows, i32 length);
static void TrsTaskFn(ecs_foreach_t* task, void** rows, i32 length);
static void CreateEntities(task_t* task, i32 begin, i32 end);
static meshid_t GenSphereMesh(float r, i32 steps);
static void RegenMesh(void);
static void RegenAlbedo(void);

// ----------------------------------------------------------------------------

static framebuf_t ms_buffer;
static rcmdqueue_t ms_queue;
static task_t ms_rastask;
static ecs_foreach_t ms_cmdtask;
static ecs_foreach_t ms_l2wtask;
static meshid_t ms_meshid;
static textureid_t ms_albedoid;
static prng_t ms_prng;
static float3 ms_modelTranslation = { 0.0f, 0.0f, 0.0f };
static float3 ms_modelForward = { 0.0f, 0.0f, -1.0f };
static float3 ms_modelUp = { 0.0f, 1.0f, 0.0f };
static float3 ms_modelScale = { 1.0f, 1.0f, 1.0f };

// ----------------------------------------------------------------------------

void render_sys_init(void)
{
    ms_prng = prng_create();

    framebuf_create(&ms_buffer, kDrawWidth, kDrawHeight);
    rcmdqueue_create(&ms_queue);
    screenblit_init(kDrawWidth, kDrawHeight);

    // demo stuff
    //CreateEntities(NULL, 0, 1 << 20);
    meshid_t sphere = GenSphereMesh(3.0, 16);
    mesh_destroy(ms_meshid);
    ms_meshid = sphere;
    RegenAlbedo();
}

ProfileMark(pm_update, render_sys_update)
void render_sys_update(void)
{
    ProfileBegin(pm_update);

    camera_t camera;
    camera_get(&camera);
    rcmdbuf_t* cmdbuf = rcmdbuf_create();
    rcmd_clear(cmdbuf, f4_v(0.01f, 0.012f, 0.022f, 0.0f), camera.nearFar.y);
    rcmd_view(cmdbuf, camera);

    material_t material =
    {
        .st = f4_v(1.0f, 1.0f, 0.0f, 0.0f),
        .albedo = ms_albedoid,
    };
    quat modelRotation = quat_lookat(f3_normalize(ms_modelForward), f3_normalize(ms_modelUp));
    float4x4 M = f4x4_trs(ms_modelTranslation, modelRotation, ms_modelScale);
    rcmd_draw(cmdbuf, M, ms_meshid, material);
    rcmd_resolve(cmdbuf, 1.0f);

    rcmdqueue_submit(&ms_queue, cmdbuf);

    ecs_foreach(&ms_l2wtask, kLocalToWorldFilter, (compflag_t) { 0 }, TrsTaskFn);
    ecs_foreach(&ms_cmdtask, kDrawableFilter, (compflag_t) { 0 }, DrawableTaskFn);
    task_submit(&ms_rastask, RasterizeTaskFn, kTileCount);

    task_sys_schedule();

    ProfileEnd(pm_update);
}

ProfileMark(pm_present, render_sys_present)
void render_sys_present(void)
{
    ProfileBegin(pm_present);

    task_await((task_t*)&ms_rastask);
    screenblit_blit(ms_buffer.color, kDrawWidth, kDrawHeight);

    ProfileEnd(pm_present);
}

void render_sys_shutdown(void)
{
    task_sys_schedule();
    screenblit_shutdown();
    task_await((task_t*)&ms_cmdtask);
    task_await((task_t*)&ms_rastask);
    framebuf_destroy(&ms_buffer);
    rcmdqueue_destroy(&ms_queue);
    mesh_destroy(ms_meshid);
    texture_destroy(ms_albedoid);
}

ProfileMark(pm_gui, render_sys_gui)
void render_sys_gui(bool* pEnabled)
{
    ProfileBegin(pm_gui);

    if (igBegin("RenderSystem", pEnabled, 0))
    {
        igSliderFloat3("translation", &ms_modelTranslation.x, -10.0f, 10.0f);
        igSliderFloat3("rotation forward", &ms_modelForward.x, -10.0f, 10.0f);
        igSliderFloat3("rotation up", &ms_modelUp.x, -10.0f, 10.0f);
        igSliderFloat3("scale", &ms_modelScale.x, -10.0f, 10.0f);
        if (igButton("Regen Mesh"))
        {
            RegenMesh();
        }
        if (igButton("Regen Albedo"))
        {
            RegenAlbedo();
        }
        if (igButton("Sphere"))
        {
            meshid_t sphere = GenSphereMesh(3.0, 64);
            mesh_destroy(ms_meshid);
            ms_meshid = sphere;
        }
    }
    igEnd();

    ProfileEnd(pm_gui);
}

// ----------------------------------------------------------------------------

pim_optimize
pim_inline int2 VEC_CALL GetTile(i32 i)
{
    i32 x = (i % kTilesPerDim);
    i32 y = (i / kTilesPerDim);
    return (int2) { x * kTileWidth, y * kTileHeight };
}

pim_optimize
pim_inline float2 VEC_CALL ScreenToUnorm(int2 screen)
{
    const float2 kRcpScreen = { 1.0f / kDrawWidth, 1.0f / kDrawHeight };
    return f2_mul(i2_f2(screen), kRcpScreen);
}

pim_optimize
pim_inline float2 VEC_CALL ScreenToSnorm(int2 screen)
{
    return f2_snorm(ScreenToUnorm(screen));
}

pim_optimize
pim_inline i32 VEC_CALL SnormToIndex(float2 s)
{
    const float2 kScale = { kDrawWidth, kDrawHeight };
    int2 i2 = f2_i2(f2_mul(f2_unorm(s), kScale));
    i32 i = i2.x + i2.y * kDrawWidth;
    ASSERT((u32)i < (u32)kDrawPixels);
    return i;
}

pim_optimize
pim_inline float2 VEC_CALL TileMin(int2 tile)
{
    return ScreenToSnorm(tile);
}

pim_optimize
pim_inline float2 VEC_CALL TileMax(int2 tile)
{
    tile.x += kTileWidth;
    tile.y += kTileHeight;
    return ScreenToSnorm(tile);
}

pim_optimize
pim_inline float2 VEC_CALL TileCenter(int2 tile)
{
    tile.x += (kTileWidth >> 1);
    tile.y += (kTileHeight >> 1);
    return ScreenToSnorm(tile);
}

pim_optimize
pim_inline float2 VEC_CALL TransformUv(float2 uv, float4 st)
{
    uv.x = uv.x * st.x + st.z;
    uv.y = uv.y * st.y + st.w;
    return uv;
}

pim_optimize
pim_inline float4 VEC_CALL Tex_Nearesti2(texture_t texture, int2 coord)
{
    coord.x = coord.x & (texture.width - 1);
    coord.y = coord.y & (texture.height - 1);
    i32 i = coord.x + coord.y * texture.width;
    return color_f4(texture.texels[i]);
}

pim_optimize
pim_inline float4 VEC_CALL Tex_Nearestf2(texture_t texture, float2 uv)
{
    uv.x = uv.x * texture.width;
    uv.y = uv.y * texture.height;
    return Tex_Nearesti2(texture, f2_i2(uv));
}

pim_optimize
pim_inline float4 VEC_CALL Tex_Bilinearf2(texture_t texture, float2 uv)
{
    uv.x = uv.x * texture.width;
    uv.y = uv.y * texture.height;
    float2 frac = f2_frac(uv);
    int2 tl = f2_i2(uv);
    float4 a = Tex_Nearesti2(texture, tl);
    float4 b = Tex_Nearesti2(texture, (int2) { tl.x + 1, tl.y + 0 });
    float4 c = Tex_Nearesti2(texture, (int2) { tl.x + 0, tl.y + 1 });
    float4 d = Tex_Nearesti2(texture, (int2) { tl.x + 1, tl.y + 1 });
    float4 e = f4_lerp(f4_lerp(a, b, frac.x), f4_lerp(c, d, frac.x), frac.y);
    return e;
}

pim_optimize
pim_inline float4 VEC_CALL TriBounds(float4x4 VP, float3 A, float3 B, float3 C, i32 iTile)
{
    float4 a = f3_f4(A, 1.0f);
    float4 b = f3_f4(B, 1.0f);
    float4 c = f3_f4(C, 1.0f);

    a = f4x4_mul_pt(VP, a);
    b = f4x4_mul_pt(VP, b);
    c = f4x4_mul_pt(VP, c);

    a = f4_divvs(a, a.w);
    b = f4_divvs(b, b.w);
    c = f4_divvs(c, c.w);

    float4 bounds;
    bounds.x = f1_min(a.x, f1_min(b.x, c.x));
    bounds.y = f1_min(a.y, f1_min(b.y, c.y));
    bounds.z = f1_max(a.x, f1_max(b.x, c.x));
    bounds.w = f1_max(a.y, f1_max(b.y, c.y));

    int2 tile = GetTile(iTile);
    float2 tileMin = TileMin(tile);
    float2 tileMax = TileMax(tile);

    bounds.x = f1_max(bounds.x, tileMin.x);
    bounds.y = f1_max(bounds.y, tileMin.y);
    bounds.z = f1_min(bounds.z, tileMax.x);
    bounds.w = f1_min(bounds.w, tileMax.y);

    return bounds;
}

ProfileMark(pm_DrawMesh, DrawMesh)
pim_optimize
static void VEC_CALL DrawMesh(renderstate_t state, rcmd_draw_t draw)
{
    const float e = 1.0f / (1 << 10);

    ProfileBegin(pm_DrawMesh);

    mesh_t mesh;
    texture_t albedo;
    material_t material = draw.material;
    if (!mesh_get(draw.meshid, &mesh) || !texture_get(material.albedo, &albedo))
    {
        return;
    }

    framebuf_t frame = ms_buffer;
    const float4* const __restrict positions = mesh.positions;
    const float4* const __restrict normals = mesh.normals;
    const float2* const __restrict uvs = mesh.uvs;
    const i32 vertCount = mesh.length;
    const float4x4 M = draw.M;
    const camera_t camera = state.camera;

    const int2 tile = GetTile(state.iTile);
    const float aspect = (float)kDrawWidth / kDrawHeight;
    const float dx = 1.0f / kDrawWidth;
    const float dy = 1.0f / kDrawHeight;
    const float nearClip = camera.nearFar.x;
    const float farClip = camera.nearFar.y;
    const float2 slope = proj_slope(f1_radians(camera.fovy), aspect);

    const float3 fwd = quat_fwd(camera.rotation);
    const float3 right = quat_right(camera.rotation);
    const float3 up = quat_up(camera.rotation);
    const float3 eye = camera.position;

    float4x4 VP;
    {
        const float4x4 V = f4x4_lookat(eye, f3_add(eye, fwd), up);
        const float4x4 P = f4x4_perspective(f1_radians(camera.fovy), aspect, nearClip, farClip);
        VP = f4x4_mul(P, V);
    }

    const float3 tileDir = proj_dir(right, up, fwd, slope, TileCenter(tile));
    const frus_t frus = frus_new(
        eye,
        right,
        up,
        fwd,
        TileMin(tile),
        TileMax(tile),
        slope,
        camera.nearFar);

    const float4 ambient = { 0.01f, 0.005f, 0.005f, 0.005f };
    const float3 L = f3_normalize(f3_1);

    for (i32 iVert = 0; (iVert + 3) <= vertCount; iVert += 3)
    {
        const float3 A = f4_f3(f4x4_mul_pt(M, positions[iVert + 0]));
        const float3 B = f4_f3(f4x4_mul_pt(M, positions[iVert + 1]));
        const float3 C = f4_f3(f4x4_mul_pt(M, positions[iVert + 2]));

        if (f3_dot(tileDir, f3_cross(f3_sub(C, A), f3_sub(B, A))) < 0.0f)
        {
            continue;
        }

        if (sdFrusSph(frus, triToSphere(A, B, C)) > 0.0f)
        {
            continue;
        }

        const float4 bounds = TriBounds(VP, A, B, C, state.iTile);

        const float3 BA = f3_sub(B, A);
        const float3 CA = f3_sub(C, A);
        const float3 T = f3_sub(eye, A);
        const float3 Q = f3_cross(T, BA);
        const float t0 = f3_dot(CA, Q);

        for (float y = bounds.y; y < bounds.w; y += dy)
        {
            for (float x = bounds.x; x < bounds.z; x += dx)
            {
                const float2 coord = { x, y };
                const float3 rd = proj_dir(right, up, fwd, slope, coord);
                const float3 rdXca = f3_cross(rd, CA);
                const float det = f3_dot(BA, rdXca);
                if (det < e)
                {
                    continue;
                }
                const i32 iTexel = SnormToIndex(coord);

                float3 wuv;
                {
                    const float rcpDet = 1.0f / det;
                    const float t = t0 * rcpDet;
                    if (t < nearClip || t > farClip || t > frame.depth[iTexel])
                    {
                        continue;
                    }
                    const float u = f3_dot(T, rdXca) * rcpDet;
                    if (u < 0.0f)
                    {
                        continue;
                    }
                    const float v = f3_dot(rd, Q) * rcpDet;
                    if (v < 0.0f)
                    {
                        continue;
                    }
                    const float w = 1.0f - u - v;
                    if (w < 0.0f)
                    {
                        continue;
                    }
                    wuv = (float3) { w, u, v };
                    frame.depth[iTexel] = t;
                }

                // blend interpolators
                const float3 P = f4_f3(f4_blend(
                    positions[iVert + 0],
                    positions[iVert + 1],
                    positions[iVert + 2],
                    wuv));

                const float3 N = f3_normalize(f4_f3(f4_blend(
                    normals[iVert + 0],
                    normals[iVert + 1],
                    normals[iVert + 2],
                    wuv)));

                const float3 V = f3_normalize(f3_sub(eye, P));
                const float3 H = f3_normalize(f3_add(V, L));
                const float NdL = f1_saturate(f3_dot(N, L));
                const float NdH = f1_saturate(f3_dot(N, H));
                const float diffuse = NdL;
                const float specular = powf(NdH, 64.0f);

                float2 U = f2_blend(
                    uvs[iVert + 0],
                    uvs[iVert + 1],
                    uvs[iVert + 2],
                    wuv);

                U = TransformUv(U, material.st);
                const float4 alb = Tex_Bilinearf2(albedo, U);

                float4 light = f4_mul(alb, f4_addvs(ambient, diffuse));
                light = f4_addvs(light, specular);

                frame.light[iTexel] = light;
            }
        }
    }

    ProfileEnd(pm_DrawMesh);
}

ProfileMark(pm_ClearTile, ClearTile)
pim_optimize
static void VEC_CALL ClearTile(renderstate_t state, rcmd_clear_t clear)
{
    ProfileBegin(pm_ClearTile);

    framebuf_t frame = ms_buffer;
    const int2 tile = GetTile(state.iTile);
    float4* light = frame.light;
    float* depth = frame.depth;

    for (i32 ty = 0; ty < kTileHeight; ++ty)
    {
        for (i32 tx = 0; tx < kTileWidth; ++tx)
        {
            i32 i = (tile.x + tx) + (tile.y + ty) * kDrawWidth;
            light[i] = clear.color;
        }
    }

    for (i32 ty = 0; ty < kTileHeight; ++ty)
    {
        for (i32 tx = 0; tx < kTileWidth; ++tx)
        {
            i32 i = (tile.x + tx) + (tile.y + ty) * kDrawWidth;
            depth[i] = clear.depth;
        }
    }

    ProfileEnd(pm_ClearTile);
}

ProfileMark(pm_ResolveTile, ResolveTile)
pim_optimize
static void VEC_CALL ResolveTile(renderstate_t state, rcmd_resolve_t resolve)
{
    ProfileBegin(pm_ResolveTile);

    framebuf_t frame = ms_buffer;
    const int2 tile = GetTile(state.iTile);
    float4* light = frame.light;
    u32* color = frame.color;

    for (i32 ty = 0; ty < kTileHeight; ++ty)
    {
        for (i32 tx = 0; tx < kTileWidth; ++tx)
        {
            i32 i = (tile.x + tx) + (tile.y + ty) * kDrawWidth;
            color[i] = f4_rgba8(tmap4_filmic(light[i]));
        }
    }

    ProfileEnd(pm_ResolveTile);
}

ProfileMark(pm_ExecTile, ExecTile)
pim_optimize
static void ExecTile(i32 iTile)
{
    ProfileBegin(pm_ExecTile);

    renderstate_t state;
    state.viewport = (float4) { 0.0f, 0.0f, kDrawWidth, kDrawHeight };
    state.iTile = iTile;
    state.rng = prng_create();
    camera_get(&state.camera);

    rcmdbuf_t* cmdBuf = NULL;
    while ((cmdBuf = rcmdqueue_read(&ms_queue, iTile)))
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
                ClearTile(state, cmd.clear);
                break;
            case RCmdType_View:
                state.camera = cmd.view.camera;
                break;
            case RCmdType_Draw:
                DrawMesh(state, cmd.draw);
                break;
            case RCmdType_Resolve:
                ResolveTile(state, cmd.resolve);
                break;
            }
        }
    }

    ProfileEnd(pm_ExecTile);
}

pim_optimize
static void RasterizeTaskFn(task_t* task, i32 begin, i32 end)
{
    for (i32 i = begin; i < end; ++i)
    {
        ExecTile(i);
    }
}

ProfileMark(pm_DrawableTaskFn, DrawableTaskFn)
static void DrawableTaskFn(ecs_foreach_t* task, void** rows, i32 length)
{
    ProfileBegin(pm_DrawableTaskFn);

    const drawable_t* drawables = (const drawable_t*)(rows[CompId_Drawable]);
    const localtoworld_t* matrices = (const localtoworld_t*)(rows[CompId_LocalToWorld]);
    const bounds_t* bounds = (const bounds_t*)(rows[CompId_Bounds]);
    ASSERT(length > 0);
    ASSERT(drawables);
    ASSERT(matrices);
    ASSERT(bounds);

    // TODO: frustum culling
    // TODO: visibility culling
    rcmdbuf_t* cmds = rcmdbuf_create();

    camera_t camera;
    camera_get(&camera);
    rcmd_view(cmds, camera);

    for (i32 i = 0; i < length; ++i)
    {
        rcmd_draw(cmds, matrices[i].Value, drawables[i].mesh, drawables[i].material);
    }

    rcmdqueue_submit(&ms_queue, cmds);

    ProfileEnd(pm_DrawableTaskFn);
}

ProfileMark(pm_TrsTaskFn, TrsTaskFn)
static void TrsTaskFn(ecs_foreach_t* task, void** rows, i32 length)
{
    ProfileBegin(pm_TrsTaskFn);

    const position_t* positions = (const position_t*)(rows[CompId_Position]);
    const rotation_t* rotations = (const rotation_t*)(rows[CompId_Rotation]);
    const scale_t* scales = (const scale_t*)(rows[CompId_Scale]);
    localtoworld_t* matrices = (localtoworld_t*)(rows[CompId_LocalToWorld]);
    ASSERT(matrices);
    ASSERT(length > 0);

    i32 code = 0;
    code |= positions ? 1 : 0;
    code |= rotations ? 2 : 0;
    code |= scales ? 4 : 0;

    switch (code)
    {
    default:
    case 0:
    {
        for (i32 i = 0; i < length; ++i)
        {
            matrices[i].Value = f4x4_id;
        }
    }
    break;
    case 1:
    {
        for (i32 i = 0; i < length; ++i)
        {
            matrices[i].Value = f4x4_trs(positions[i].Value, quat_id, f3_1);
        }
    }
    break;
    case 2:
    {
        for (i32 i = 0; i < length; ++i)
        {
            matrices[i].Value = f4x4_trs(f3_0, rotations[i].Value, f3_1);
        }
    }
    break;
    case 4:
    {
        for (i32 i = 0; i < length; ++i)
        {
            matrices[i].Value = f4x4_trs(f3_0, quat_id, scales[i].Value);
        }
    }
    break;
    case (1 | 2):
    {
        for (i32 i = 0; i < length; ++i)
        {
            matrices[i].Value = f4x4_trs(positions[i].Value, rotations[i].Value, f3_1);
        }
    }
    break;
    case (1 | 4):
    {
        for (i32 i = 0; i < length; ++i)
        {
            matrices[i].Value = f4x4_trs(positions[i].Value, quat_id, scales[i].Value);
        }
    }
    break;
    case (2 | 4):
    {
        for (i32 i = 0; i < length; ++i)
        {
            matrices[i].Value = f4x4_trs(f3_0, rotations[i].Value, scales[i].Value);
        }
    }
    break;
    case (1 | 2 | 4):
    {
        for (i32 i = 0; i < length; ++i)
        {
            matrices[i].Value = f4x4_trs(positions[i].Value, rotations[i].Value, scales[i].Value);
        }
    }
    break;
    }

    ProfileEnd(pm_TrsTaskFn);
}

static void CreateEntities(task_t* task, i32 begin, i32 end)
{
    prng_t rng = prng_create();
    compflag_t all = compflag_create(6, CompId_Position, CompId_LocalToWorld, CompId_Drawable, CompId_Bounds);
    compflag_t some = compflag_create(1, CompId_Position);
    for (i32 i = begin; i < end; ++i)
    {
        ecs_create(prng_i32(&rng) & 1 ? all : some);
    }
}

static meshid_t GenSphereMesh(float r, i32 steps)
{
    const i32 vsteps = steps;       // divisions along y axis
    const i32 hsteps = steps * 2;   // divisions along x-z plane
    const float dv = kPi / vsteps;
    const float dh = kTau / hsteps;

    const i32 maxlen = 6 * vsteps * hsteps;
    i32 length = 0;
    float4* positions = perm_malloc(sizeof(*positions) * maxlen);
    float4* normals = perm_malloc(sizeof(*normals) * maxlen);
    float2* uvs = perm_malloc(sizeof(*uvs) * maxlen);

    for (i32 v = 0; v < vsteps; ++v)
    {
        float theta1 = v * dv;
        float theta2 = (v + 1) * dv;
        float st1 = sinf(theta1);
        float ct1 = cosf(theta1);
        float st2 = sinf(theta2);
        float ct2 = cosf(theta2);

        for (i32 h = 0; h < hsteps; ++h)
        {
            float phi1 = h * dh;
            float phi2 = (h + 1) * dh;
            float sp1 = sinf(phi1);
            float cp1 = cosf(phi1);
            float sp2 = sinf(phi2);
            float cp2 = cosf(phi2);

            // p2  p1
            // |   |
            // 2---1 -- t1
            // |\  |
            // | \ |
            // 3---4 -- t2

            float2 u1 = f2_v(phi1 / kTau, 1.0f - theta1 / kPi);
            float2 u2 = f2_v(phi2 / kTau, 1.0f - theta1 / kPi);
            float2 u3 = f2_v(phi2 / kTau, 1.0f - theta2 / kPi);
            float2 u4 = f2_v(phi1 / kTau, 1.0f - theta2 / kPi);

            float4 n1 = f4_v(st1 * cp1, ct1, st1 * sp1, 0.0f);
            float4 n2 = f4_v(st1 * cp2, ct1, st1 * sp2, 0.0f);
            float4 n3 = f4_v(st2 * cp2, ct2, st2 * sp2, 0.0f);
            float4 n4 = f4_v(st2 * cp1, ct2, st2 * sp1, 0.0f);

            float4 v1 = f4_mulvs(n1, r);
            float4 v2 = f4_mulvs(n2, r);
            float4 v3 = f4_mulvs(n3, r);
            float4 v4 = f4_mulvs(n4, r);

            const i32 back = length;
            if (v == 0)
            {
                length += 3;
                ASSERT(length <= maxlen);

                positions[back + 0] = v1;
                positions[back + 1] = v3;
                positions[back + 2] = v4;

                normals[back + 0] = n1;
                normals[back + 1] = n3;
                normals[back + 2] = n4;

                uvs[back + 0] = u1;
                uvs[back + 1] = u3;
                uvs[back + 2] = u4;
            }
            else if ((v + 1) == vsteps)
            {
                length += 3;
                ASSERT(length <= maxlen);

                positions[back + 0] = v3;
                positions[back + 1] = v1;
                positions[back + 2] = v2;

                normals[back + 0] = n3;
                normals[back + 1] = n1;
                normals[back + 2] = n2;

                uvs[back + 0] = u3;
                uvs[back + 1] = u1;
                uvs[back + 2] = u2;
            }
            else
            {
                length += 6;
                ASSERT(length <= maxlen);

                positions[back + 0] = v1;
                positions[back + 1] = v2;
                positions[back + 2] = v4;

                normals[back + 0] = n1;
                normals[back + 1] = n2;
                normals[back + 2] = n4;

                uvs[back + 0] = u1;
                uvs[back + 1] = u2;
                uvs[back + 2] = u4;

                positions[back + 3] = v2;
                positions[back + 4] = v3;
                positions[back + 5] = v4;

                normals[back + 3] = n2;
                normals[back + 4] = n3;
                normals[back + 5] = n4;

                uvs[back + 3] = u2;
                uvs[back + 4] = u3;
                uvs[back + 5] = u4;
            }
        }
    }

    return mesh_create(&(mesh_t)
    {
        .length = length,
        .positions = positions,
        .normals = normals,
        .uvs = uvs,
    });
}

static void RegenMesh(void)
{
    mesh_t mesh;
    mesh.length = (1 + (prng_i32(&ms_prng) & 15)) * 3;
    mesh.positions = perm_malloc(sizeof(*mesh.positions) * mesh.length);
    mesh.normals = perm_malloc(sizeof(*mesh.normals) * mesh.length);
    mesh.uvs = perm_malloc(sizeof(*mesh.uvs) * mesh.length);

    prng_t rng = ms_prng;
    for (i32 i = 0; i < mesh.length; ++i)
    {
        float4 position;
        position.x = f1_lerp(-5.0f, 5.0f, prng_f32(&rng));
        position.y = f1_lerp(-5.0f, 5.0f, prng_f32(&rng));
        position.z = f1_lerp(-5.0f, 5.0f, prng_f32(&rng));
        position.w = 1.0f;
        float4 normal;
        normal.x = f1_lerp(-1.0f, 1.0f, prng_f32(&rng));
        normal.y = f1_lerp(-1.0f, 1.0f, prng_f32(&rng));
        normal.z = f1_lerp(-1.0f, 1.0f, prng_f32(&rng));
        normal.w = 0.0f;
        normal = f4_normalize3(normal);
        float2 uv;
        switch (i % 3)
        {
        case 0:
            uv = f2_v(0.0f, 0.0f);
            break;
        case 1:
            uv = f2_v(1.0f, 0.0f);
            break;
        case 2:
            uv = f2_v(0.0f, 1.0f);
            break;
        }
        mesh.positions[i] = position;
        mesh.normals[i] = normal;
        mesh.uvs[i] = uv;
    }
    ms_prng = rng;

    meshid_t newid = mesh_create(&mesh);
    meshid_t oldid = ms_meshid;
    ms_meshid = newid;
    mesh_destroy(oldid);
}

static void RegenAlbedo(void)
{
    texture_t albedo;
    const i32 size = 1 << 8;
    albedo.width = size;
    albedo.height = size;
    albedo.texels = perm_malloc(sizeof(*albedo.texels) * size * size);

    prng_t rng = ms_prng;
    for (i32 y = 0; y < size; ++y)
    {
        for (i32 x = 0; x < size; ++x)
        {
            const i32 i = x + y * size;
            bool c0 = (x & 7) < 4;
            bool c1 = (y & 7) < 4;
            u32 a = c1 ? 0xffffffff : 0;
            u32 b = ~a;
            albedo.texels[i] = c0 ? a : b;
        }
    }
    ms_prng = rng;

    textureid_t newid = texture_create(&albedo);
    textureid_t oldid = ms_albedoid;
    ms_albedoid = newid;
    texture_destroy(oldid);
}
