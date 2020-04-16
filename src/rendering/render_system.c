#include "rendering/render_system.h"

#include "allocator/allocator.h"
#include "common/time.h"
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
#include "ui/cimgui.h"

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
static void RegenMesh(void);

// ----------------------------------------------------------------------------

static framebuf_t ms_buffer;
static rcmdqueue_t ms_queue;
static task_t ms_rastask;
static ecs_foreach_t ms_cmdtask;
static ecs_foreach_t ms_l2wtask;
static meshid_t ms_meshid;
static prng_t ms_prng;
static float ms_dt;
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
    RegenMesh();
}

void render_sys_update(void)
{
    igBegin("RenderSystem", NULL, 0);
    {
        float dt = (float)time_dtf();
        ms_dt = f1_lerp(ms_dt, dt, 1.0f / 60.0f);
        igValueFloat("ms", ms_dt * 1000.0f, NULL);
        igValueFloat("fps", 1.0f / ms_dt, NULL);
        igSliderFloat3("translation", &ms_modelTranslation.x, -10.0f, 10.0f, NULL, 1.0f);
        igSliderFloat3("rotation forward", &ms_modelForward.x, -10.0f, 10.0f, NULL, 1.0f);
        igSliderFloat3("rotation up", &ms_modelUp.x, -10.0f, 10.0f, NULL, 1.0f);
        igSliderFloat3("scale", &ms_modelScale.x, -10.0f, 10.0f, NULL, 1.0f);
        if (igButton("Regen Mesh"))
        {
            RegenMesh();
        }
    }
    igEnd();

    camera_t camera;
    camera_get(&camera);
    rcmdbuf_t* cmdbuf = rcmdbuf_create();
    rcmd_clear(cmdbuf, f4_rgba8(f4_v(0.02f, 0.05f, 0.1f, 1.0f)), camera.nearFar.y);
    rcmd_view(cmdbuf, camera);

    material_t material = { 0 };
    quat modelRotation = quat_lookat(f3_normalize(ms_modelForward), f3_normalize(ms_modelUp));
    float4x4 M = f4x4_trs(ms_modelTranslation, modelRotation, ms_modelScale);
    rcmd_draw(cmdbuf, M, ms_meshid, material);

    rcmdqueue_submit(&ms_queue, cmdbuf);

    ecs_foreach(&ms_l2wtask, kLocalToWorldFilter, (compflag_t) { 0 }, TrsTaskFn);
    ecs_foreach(&ms_cmdtask, kDrawableFilter, (compflag_t) { 0 }, DrawableTaskFn);
    task_submit(&ms_rastask, RasterizeTaskFn, kTileCount);

    task_sys_schedule();
}

void render_sys_present(void)
{
    task_await((task_t*)&ms_rastask);
    screenblit_blit(ms_buffer.color, kDrawWidth, kDrawHeight);
}

void render_sys_shutdown(void)
{
    task_sys_schedule();
    screenblit_shutdown();
    task_await((task_t*)&ms_cmdtask);
    task_await((task_t*)&ms_rastask);
    framebuf_destroy(&ms_buffer);
    rcmdqueue_destroy(&ms_queue);
}

// ----------------------------------------------------------------------------

pim_inline int2 VEC_CALL GetTile(i32 i)
{
    i32 x = (i % kTilesPerDim);
    i32 y = (i / kTilesPerDim);
    return (int2) { x * kTileWidth, y * kTileHeight };
}

static void VEC_CALL DrawMesh(renderstate_t state, rcmd_draw_t draw)
{
    const float e = 1.0f / (1 << 10);

    mesh_t mesh;
    if (!mesh_get(draw.meshid, &mesh))
    {
        return;
    }

    framebuf_t frame = ms_buffer;
    const float4* const positions = mesh.positions;
    const float4* const normals = mesh.normals;
    const float2* const uvs = mesh.uvs;
    const i32 vertCount = mesh.length;
    const float4x4 M = draw.M;
    const camera_t camera = state.camera;
    // material_t material = draw.material;

    const int2 tile = GetTile(state.iTile);
    const float2 rcpScreen = { 1.0f / kDrawWidth, 1.0f / kDrawHeight };
    const float aspect = (float)kDrawWidth / (float)kDrawHeight;
    const float fovSlope = tanf(camera.fovy * 0.5f);
    const float xSlope = aspect * fovSlope;
    const float ySlope = fovSlope;

    const float3 forward = quat_fwd(camera.rotation);
    const float3 right = quat_right(camera.rotation);
    const float3 up = quat_up(camera.rotation);
    const float3 ro = camera.position;
    prng_t rng = state.rng;

    for (i32 y = 0; y < kTileHeight; ++y)
    {
        for (i32 x = 0; x < kTileWidth; ++x)
        {
            const int2 iCoord = i2_add(tile, i2_v(x, y));
            const i32 iTexel = iCoord.x + iCoord.y * kDrawWidth;
            const float2 fCoord = f2_snorm(f2_mul(i2_f2(iCoord), rcpScreen));

            const float3 rd = f3_normalize(f3_add(forward, f3_add(f3_mulvs(right, fCoord.x * xSlope), f3_mulvs(up, fCoord.y * ySlope))));

            for (i32 iVert = 0; (iVert + 3) <= vertCount; iVert += 3)
            {
                const float3 A = f4_f3(f4x4_mul_pt(M, positions[iVert + 0]));
                const float3 B = f4_f3(f4x4_mul_pt(M, positions[iVert + 1]));
                const float3 C = f4_f3(f4x4_mul_pt(M, positions[iVert + 2]));

                const float4 uvt = ray_tri_isect(ro, rd, A, B, C);
                if (uvt.w != 1.0f)
                {
                    continue;
                }
                if (uvt.z > frame.depth[iTexel] || uvt.z < camera.nearFar.x)
                {
                    continue;
                }
                frame.depth[iTexel] = uvt.z;
                // const float3 P = f3_add(ro, f3_mulvs(rd, uvt.z));

                // blend interpolators
                const float3 wuv = f3_v(1.0f - uvt.x - uvt.y, uvt.x, uvt.y);
                // const float4 N = f4_blend(NA, NB, NC, wuv);
                const float2 UA = uvs[iVert + 0];
                const float2 UB = uvs[iVert + 1];
                const float2 UC = uvs[iVert + 2];
                float2 U = f2_blend(UA, UB, UC, wuv);

                frame.color[iTexel] = f4_color(&rng, f4_v(U.x, U.y, 0.0f, 1.0f));
            }
        }
    }

    state.rng = rng;
}

static void VEC_CALL ClearTile(renderstate_t state, rcmd_clear_t clear)
{
    framebuf_t frame = ms_buffer;
    const int2 tile = GetTile(state.iTile);
    u32* color = frame.color;
    float* depth = frame.depth;

    for (i32 ty = 0; ty < kTileHeight; ++ty)
    {
        for (i32 tx = 0; tx < kTileWidth; ++tx)
        {
            i32 i = (tile.x + tx) + (tile.y + ty) * kDrawWidth;
            color[i] = clear.color;
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
}

static void ExecTile(i32 iTile)
{
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
            }
        }
    }
}

static void RasterizeTaskFn(task_t* task, i32 begin, i32 end)
{
    for (i32 i = begin; i < end; ++i)
    {
        ExecTile(i);
    }
}

static void DrawableTaskFn(ecs_foreach_t* task, void** rows, i32 length)
{
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
}

static void TrsTaskFn(ecs_foreach_t* task, void** rows, i32 length)
{
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

static void RegenMesh(void)
{
    mesh_t mesh;
    mesh.length = (1 + (prng_i32(&ms_prng) & 3)) * 3;
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