#include "rendering/render_system.h"

#include <glad/glad.h>
#include "ui/cimgui.h"

#include "allocator/allocator.h"
#include "common/time.h"
#include "components/ecs.h"
#include "rendering/framebuffer.h"
#include "rendering/constants.h"
#include "rendering/rcmd.h"
#include "rendering/camera.h"
#include "rendering/window.h"
#include "math/types.h"
#include "math/int2_funcs.h"
#include "math/float4_funcs.h"
#include "math/float3_funcs.h"
#include "math/float2_funcs.h"
#include "math/float4x4_funcs.h"
#include "math/sdf.h"
#include "io/fd.h"
#include <string.h>

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

typedef struct renderstate_s
{
    float4 viewport;
    float4x4 P;
    float4x4 V;
    prng_t rng;
    i32 iTile;
} renderstate_t;

typedef GLuint glhandle;

typedef struct glmesh_s
{
    glhandle vao;
    glhandle vbo;
    i32 vertCount;
} glmesh_t;

typedef struct vert_attrib_s
{
    i32 dimension;
    i32 offset;
} vert_attrib_t;

static glmesh_t ms_blitMesh;
static glhandle ms_blitProgram;
static glhandle ms_image;
static framebuf_t ms_buffer;
static rcmdqueue_t ms_queue;
static task_t ms_rastask;

static const compflag_t kDrawableFilter =
{
    (1 << CompId_LocalToWorld) |
    (1 << CompId_Drawable)
};
static ecs_foreach_t ms_cmdtask;

static const compflag_t kLocalToWorldFilter =
{
    (1 << CompId_Position) |
    (1 << CompId_Rotation) |
    (1 << CompId_Scale) |
    (1 << CompId_LocalToWorld)
};
static ecs_foreach_t ms_l2wtask;

static meshid_t ms_meshid;
static prng_t ms_prng;
static float ms_dt;
static float3 ms_modelTranslation = { 0.0f, 0.0f, 0.0f };
static float3 ms_modelForward = { 0.0f, 0.0f, -1.0f };
static float3 ms_modelUp = { 0.0f, 1.0f, 0.0f };
static float3 ms_modelScale = { 1.0f, 1.0f, 1.0f };

static const float kScreenMesh[] =
{
    -1.0f, -1.0f,
     1.0f, -1.0f,
     1.0f,  1.0f,
     1.0f,  1.0f,
    -1.0f,  1.0f,
    -1.0f, -1.0f
};

static const char* const kBlitVertShader[] =
{
    "#version 330 core\n",
    "layout(location = 0) in vec2 mesh;\n",
    "out vec2 uv;\n",
    "void main()\n",
    "{\n",
    "   gl_Position = vec4(mesh.xy, 0.0, 1.0);\n",
    "   uv = mesh * 0.5 + 0.5;\n",
    "}\n",
};

static const char* const kBlitFragShader[] =
{
    "#version 330 core\n",
    "in vec2 uv;\n",
    "out vec4 outColor;\n",
    "uniform sampler2D inColor;\n",
    "void main()\n",
    "{\n",
    "   outColor = texture(inColor, uv);\n",
    "}\n",
};

pim_inline int2 VEC_CALL UnormToScreen(float2 u)
{
    const float2 kScreen = { kDrawWidth, kDrawHeight };
    u = f2_floor(f2_mul(u, kScreen));
    int2 s = { (i32)u.x, (i32)u.y };
    return s;
}

pim_inline float2 VEC_CALL ScreenToUnorm(int2 s)
{
    const float2 kRcpScreen = { 1.0f / kDrawWidth, 1.0f / kDrawHeight };
    float2 u = f2_iv(s.x, s.y);
    u = f2_mul(u, kRcpScreen);
    return u;
}

pim_inline int2 VEC_CALL GetScreenTile(i32 i)
{
    i32 x = (i % kTilesPerDim);
    i32 y = (i / kTilesPerDim);
    return (int2) { x * kTileWidth, y * kTileHeight };
}

pim_inline float2 VEC_CALL GetUnormTile(i32 i)
{
    return ScreenToUnorm(GetScreenTile(i));
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
    const float4x4 M = draw.M;
    const float4x4 V = state.V;
    const float4x4 P = state.P;
    // material_t material = draw.material;

    const float2 tileMin = GetUnormTile(state.iTile);
    const float2 tileMax = f2_add(tileMin, kTileSize);
    prng_t rng = state.rng;

    for (i32 iVert = 0; (iVert + 3) <= mesh.length; iVert += 3)
    {
        // local space
        const float4 localA = mesh.positions[iVert];
        const float4 localB = mesh.positions[iVert + 1];
        const float4 localC = mesh.positions[iVert + 2];

        // model
        const float4 worldA = f4x4_mul_pt(M, localA);
        const float4 worldB = f4x4_mul_pt(M, localB);
        const float4 worldC = f4x4_mul_pt(M, localC);

        // view
        const float4 viewA = f4x4_mul_pt(V, worldA);
        const float4 viewB = f4x4_mul_pt(V, worldB);
        const float4 viewC = f4x4_mul_pt(V, worldC);

        // projection
        float4 A = f4x4_mul_pt(P, viewA);
        float4 B = f4x4_mul_pt(P, viewB);
        float4 C = f4x4_mul_pt(P, viewC);

        if (A.w < e && B.w < e && C.w < e)
        {
            continue;
        }

        // guard against division by very small values
        A.w = f1_max(A.w, e);
        B.w = f1_max(B.w, e);
        C.w = f1_max(C.w, e);

        // perspective divide
        A = f4_divvs(A, A.w);
        B = f4_divvs(B, B.w);
        C = f4_divvs(C, C.w);

        // snorm clip space to unorm clip space
        A = f4_unorm(A);
        B = f4_unorm(B);
        C = f4_unorm(C);

        // 2D triangle and area
        const float2 a = f4_f2(A);
        const float2 b = f4_f2(B);
        const float2 c = f4_f2(C);
        const float area = tri_area2D(a, b, c);

        // discard backfaces
        if (area < e)
        {
            continue;
        }

        // discard triangles outside unorm clip space of tile
        if (f3_cliptest(f3_v(A.z, B.z, C.z), 0.0f, 1.0f))
        {
            continue;
        }
        if (f3_cliptest(f3_v(a.x, b.x, c.x), tileMin.x, tileMax.x))
        {
            continue;
        }
        if (f3_cliptest(f3_v(a.y, b.y, c.y), tileMin.y, tileMax.y))
        {
            continue;
        }

        // interpolated attributes
        const float4 NA = mesh.normals[iVert];
        const float4 NB = mesh.normals[iVert + 1];
        const float4 NC = mesh.normals[iVert + 2];

        // rasterization bounds
        const float yMin = f1_max(tileMin.y, f3_hmin(f3_v(a.y, b.y, c.y)));
        const float yMax = f1_min(tileMax.y, f3_hmax(f3_v(a.y, b.y, c.y)));
        const float xMin = f1_max(tileMin.x, f3_hmin(f3_v(a.x, b.x, c.x)));
        const float xMax = f1_min(tileMax.x, f3_hmax(f3_v(a.x, b.x, c.x)));

        // constants
        const float dx = 1.0f / kDrawWidth;
        const float dy = 1.0f / kDrawHeight;
        const float2 kDrawSize = { kDrawWidth, kDrawHeight };
        const int2 kCoordStride = { 1, kDrawWidth };
        const float dz = 0xffff;
        const float rcpArea = 1.0f / area;

        for (float y = yMin; y <= yMax; y += dy)
        {
            for (float x = xMin; x <= xMax; x += dx)
            {
                const float2 p = f2_v(x, y);

                // barycentrics
                const float3 wuv = bary2D(a, b, c, rcpArea, p);

                // barycentric clip
                if (f3_any(f3_ltvs(wuv, 0.0f)))
                {
                    continue;
                }

                const float4 P = f4_blend(A, B, C, wuv);
                const u16 Z = (u16)(P.z * dz);

                // depth clip
                if (P.z > 1.0f || P.z < 0.0f)
                {
                    continue;
                }

                // depth test
                const i32 iTexel = i2_dot(f2_i2(f2_mul(p, kDrawSize)), kCoordStride);
                ASSERT((u32)iTexel < (u32)kDrawPixels);
                if (Z >= frame.depth[iTexel])
                {
                    continue;
                }

                // blend interpolators
                const float4 N = f4_blend(NA, NB, NC, wuv);

                const u16 C = f4_color(&rng, N);
                frame.depth[iTexel] = Z;
                frame.color[iTexel] = C;
            }
        }
    }

    state.rng = rng;
}

static void VEC_CALL ClearTile(renderstate_t state, rcmd_clear_t clear)
{
    framebuf_t frame = ms_buffer;
    const int2 tile = GetScreenTile(state.iTile);
    u16* __restrict color = frame.color;
    u16* __restrict depth = frame.depth;
    const u16 C = clear.color;
    const u16 Z = clear.depth;

    for (i32 ty = 0; ty < kTileHeight; ++ty)
    {
        for (i32 tx = 0; tx < kTileWidth; ++tx)
        {
            i32 i = (tile.x + tx) + (tile.y + ty) * kDrawWidth;
            color[i] = C;
        }
    }

    for (i32 ty = 0; ty < kTileHeight; ++ty)
    {
        for (i32 tx = 0; tx < kTileWidth; ++tx)
        {
            i32 i = (tile.x + tx) + (tile.y + ty) * kDrawWidth;
            depth[i] = Z;
        }
    }
}

static void ExecTile(i32 iTile)
{
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
    state.rng = prng_create();

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

static void RasterizeTaskFn(task_t* task, i32 begin, i32 end)
{
    for (i32 i = begin; i < end; ++i)
    {
        ExecTile(i);
    }
}

static void DrawableTaskFn(ecs_foreach_t* task, void** rows, i32 length)
{
    drawable_t* drawables = (drawable_t*)(rows[CompId_Drawable]);
    localtoworld_t* matrices = (localtoworld_t*)(rows[CompId_LocalToWorld]);
    ASSERT(length > 0);
    ASSERT(drawables);
    ASSERT(matrices);

    // TODO: frustum culling
    // TODO: visibility culling
    rcmdbuf_t* cmds = rcmdbuf_create();

    camera_t camera;
    camera_get(&camera);
    float aspect = (float)window_width() / (float)window_height();
    float fovy = f1_radians(camera.fovy);
    float4x4 P = f4x4_perspective(fovy, aspect, camera.nearFar.x, camera.nearFar.y);
    float4x4 V = f4x4_lookat(camera.position, f3_add(camera.position, quat_fwd(camera.rotation)), quat_up(camera.rotation));
    rcmd_view(cmds, V, P);

    for (i32 i = 0; i < length; ++i)
    {
        rcmd_draw(cmds, matrices[i].Value, drawables[i].mesh, drawables[i].material);
    }

    rcmdqueue_submit(&ms_queue, cmds);
}

static void LocalToWorldTaskFn(ecs_foreach_t* task, void** rows, i32 length)
{
    position_t* positions = (position_t*)(rows[CompId_Position]);
    rotation_t* rotations = (rotation_t*)(rows[CompId_Rotation]);
    scale_t* scales = (scale_t*)(rows[CompId_Scale]);
    localtoworld_t* matrices = (localtoworld_t*)(rows[CompId_LocalToWorld]);
    ASSERT(positions);
    ASSERT(rotations);
    ASSERT(scales);
    ASSERT(matrices);
    ASSERT(length > 0);

    for (i32 i = 0; i < length; ++i)
    {
        matrices[i].Value = f4x4_trs(f4_f3(positions[i].Value), rotations[i].Value, f4_f3(scales[i].Value));
    }
}

static void CreateEntities(task_t* task, i32 begin, i32 end)
{
    prng_t rng = prng_create();
    compflag_t all = compflag_create(5, CompId_Position, CompId_Rotation, CompId_Scale, CompId_LocalToWorld, CompId_Drawable);
    compflag_t some = compflag_create(1, CompId_Position);
    for (i32 i = begin; i < end; ++i)
    {
        ecs_create(prng_i32(&rng) & 1 ? all : some);
    }
}

static void RegenMesh(void)
{
    mesh_t mesh;
    mesh.length = (1 + (prng_i32(&ms_prng) & 31)) * 3;
    mesh.positions = perm_malloc(sizeof(*mesh.positions) * mesh.length);
    mesh.normals = perm_malloc(sizeof(*mesh.normals) * mesh.length);
    mesh.uvs = perm_malloc(sizeof(*mesh.uvs) * mesh.length);

    prng_t rng = ms_prng;
    for (i32 i = 0; i < mesh.length; ++i)
    {
        float4 position;
        position.x = f1_lerp(-2.0f, 2.0f, prng_f32(&rng));
        position.y = f1_lerp(-2.0f, 2.0f, prng_f32(&rng));
        position.z = f1_lerp(-2.0f, 2.0f, prng_f32(&rng));
        position.w = 1.0f;
        float4 normal;
        normal.x = f1_lerp(0.0001f, 1.0f, prng_f32(&rng));
        normal.y = f1_lerp(0.0001f, 1.0f, prng_f32(&rng));
        normal.z = f1_lerp(0.0001f, 1.0f, prng_f32(&rng));
        normal.w = 0.0f;
        float2 uv;
        uv.x = prng_f32(&rng);
        uv.y = prng_f32(&rng);
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

static glhandle CreateGlProgram(
    i32 vsLines, const char* const * const vs,
    i32 fsLines, const char* const * const fs);
static void DestroyGlProgram(glhandle* pProg);
static void SetupTextureUnit(glhandle prog, const char* texName, i32 unit);

static glhandle CreateGlTexture(i32 width, i32 height);
static void UpdateGlTexture(
    glhandle hdl, i32 width, i32 height, const void* data);
static void DestroyGlTexture(glhandle* pHdl);
static void BindGlTexture(glhandle hdl, i32 unit);

static glmesh_t CreateGlMesh(
    i32 bytes, const void* data,
    i32 stride,
    i32 attribCount, const vert_attrib_t* attribs);
static void DrawGlMesh(glmesh_t mesh);
static void DestroyGlMesh(glmesh_t* pMesh);

void render_sys_init(void)
{
    ms_prng = prng_create();

    framebuf_create(&ms_buffer, kDrawWidth, kDrawHeight);
    rcmdqueue_create(&ms_queue);

    // setup gl texture blit
    ms_image = CreateGlTexture(kDrawWidth, kDrawHeight);
    ASSERT(ms_image);
    const vert_attrib_t attribs[] =
    {
        { .dimension = 2, .offset = 0 },
    };
    ms_blitMesh = CreateGlMesh(
        sizeof(kScreenMesh), kScreenMesh,
        sizeof(float2),
        NELEM(attribs), attribs);
    ms_blitProgram = CreateGlProgram(
        NELEM(kBlitVertShader), kBlitVertShader,
        NELEM(kBlitFragShader), kBlitFragShader);
    ASSERT(ms_blitProgram);
    SetupTextureUnit(ms_blitProgram, "inColor", 0);

    // demo stuff
    //CreateEntities(NULL, 0, 1 << 20);
    RegenMesh();
}

void render_sys_update(void)
{
    camera_t camera;
    camera_get(&camera);

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

    rcmdbuf_t* cmdbuf = rcmdbuf_create();
    rcmd_clear(cmdbuf, 0x0000, 0xffff);

    float aspect = (float)window_width() / (float)window_height();
    float fovy = f1_radians(camera.fovy);
    float4x4 P = f4x4_perspective(fovy, aspect, camera.nearFar.x, camera.nearFar.y);
    float4x4 V = f4x4_lookat(camera.position, f3_add(camera.position, quat_fwd(camera.rotation)), quat_up(camera.rotation));
    rcmd_view(cmdbuf, V, P);

    material_t material = { 0 };
    quat modelRotation = quat_lookat(f3_normalize(ms_modelForward), f3_normalize(ms_modelUp));
    float4x4 M = f4x4_trs(ms_modelTranslation, modelRotation, ms_modelScale);
    rcmd_draw(cmdbuf, M, ms_meshid, material);

    rcmdqueue_submit(&ms_queue, cmdbuf);

    ecs_foreach(&ms_l2wtask, kLocalToWorldFilter, (compflag_t) { 0 }, LocalToWorldTaskFn);
    ecs_foreach(&ms_cmdtask, kDrawableFilter, (compflag_t) { 0 }, DrawableTaskFn);
    task_submit(&ms_rastask, RasterizeTaskFn, kTileCount);

    task_sys_schedule();
}

void render_sys_present(void)
{
    task_await((task_t*)&ms_rastask);
    UpdateGlTexture(ms_image, kDrawWidth, kDrawHeight, ms_buffer.color);

    glViewport(0, 0, window_width(), window_height());
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glUseProgram(ms_blitProgram);
    BindGlTexture(ms_image, 0);
    DrawGlMesh(ms_blitMesh);
}

void render_sys_shutdown(void)
{
    task_sys_schedule();

    task_await((task_t*)&ms_cmdtask);
    task_await((task_t*)&ms_rastask);
    framebuf_destroy(&ms_buffer);
    rcmdqueue_destroy(&ms_queue);
    DestroyGlTexture(&ms_image);
    DestroyGlMesh(&ms_blitMesh);
    DestroyGlProgram(&ms_blitProgram);
}

static glhandle CreateGlTexture(i32 width, i32 height)
{
    glhandle hdl = 0;
    glGenTextures(1, &hdl);
    ASSERT(hdl);
    ASSERT(!glGetError());
    glBindTexture(GL_TEXTURE_2D, hdl);
    ASSERT(!glGetError());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    ASSERT(!glGetError());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    ASSERT(!glGetError());
    glTexImage2D(
        GL_TEXTURE_2D,              // target
        0,                          // level
        GL_RGB5_A1,                 // internalformat
        width,                      // width
        height,                     // height
        GL_FALSE,                   // border
        GL_RGBA,                    // format
        GL_UNSIGNED_SHORT_5_5_5_1,  // type
        NULL);                      // data
    ASSERT(!glGetError());
    return hdl;
}

static void UpdateGlTexture(
    glhandle hdl,
    i32 width,
    i32 height,
    const void* data)
{
    ASSERT(hdl);
    ASSERT(data);
    glBindTexture(GL_TEXTURE_2D, hdl);
    ASSERT(!glGetError());
    glTexSubImage2D(
        GL_TEXTURE_2D,              // target
        0,                          // mip level
        0,                          // xoffset
        0,                          // yoffset
        width,                      // width
        height,                     // height
        GL_RGBA,                    // format
        GL_UNSIGNED_SHORT_5_5_5_1,  // type
        data);                      // data
    ASSERT(!glGetError());
}

static void DestroyGlTexture(glhandle* pHdl)
{
    ASSERT(pHdl);
    glhandle hdl = *pHdl;
    *pHdl = 0;
    if (hdl)
    {
        glDeleteTextures(1, &hdl);
        ASSERT(!glGetError());
    }
}

static void BindGlTexture(glhandle hdl, i32 index)
{
    ASSERT(hdl);
    ASSERT(index >= 0 && index < 8);
    glActiveTexture(GL_TEXTURE0 + index);
    ASSERT(!glGetError());
    glBindTexture(GL_TEXTURE_2D, hdl);
    ASSERT(!glGetError());
}

static glmesh_t CreateGlMesh(
    i32 bytes, const void* data,
    i32 stride,
    i32 attribCount, const vert_attrib_t* attribs)
{
    ASSERT(bytes > 0);
    ASSERT(data);
    ASSERT(attribCount > 0);
    ASSERT(attribs);

    i32 vao = 0;
    i32 vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    ASSERT(vao);
    ASSERT(vbo);
    ASSERT(!glGetError());

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    ASSERT(!glGetError());
    for (i32 i = 0; i < attribCount; ++i)
    {
        glEnableVertexAttribArray(i);
        ASSERT(!glGetError());
        glVertexAttribPointer(
            i,                              // index
            attribs[i].dimension,           // dimension
            GL_FLOAT,                       // type
            GL_FALSE,                       // normalized
            stride,                         // bytes between vertices
            (void*)(isize)(attribs[i].offset));    // offset of attribute
        ASSERT(!glGetError());
    }
    glBufferData(GL_ARRAY_BUFFER, bytes, data, GL_STATIC_DRAW);
    ASSERT(!glGetError());

    glmesh_t mesh;
    mesh.vao = vao;
    mesh.vbo = vbo;
    mesh.vertCount = bytes / stride;

    return mesh;
}

static void DrawGlMesh(glmesh_t mesh)
{
    ASSERT(mesh.vao);
    ASSERT(mesh.vbo);
    ASSERT(mesh.vertCount > 0);
    glBindVertexArray(mesh.vao);
    ASSERT(!glGetError());
    glDrawArrays(GL_TRIANGLES, 0, mesh.vertCount);
    ASSERT(!glGetError());
}

static void DestroyGlMesh(glmesh_t* pMesh)
{
    ASSERT(pMesh);
    glhandle vao = pMesh->vao;
    glhandle vbo = pMesh->vbo;
    pMesh->vao = 0;
    pMesh->vbo = 0;
    pMesh->vertCount = 0;
    if (vbo)
    {
        glDeleteBuffers(1, &vbo);
        ASSERT(!glGetError());
    }
    if (vao)
    {
        glDeleteVertexArrays(1, &vao);
        ASSERT(!glGetError());
    }
}

static char* GetShaderLog(glhandle shader)
{
    ASSERT(shader);
    i32 status = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    ASSERT(!glGetError());
    if (!status)
    {
        i32 loglen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &loglen);
        ASSERT(!glGetError());
        char* infolog = pim_calloc(EAlloc_Temp, loglen + 1);
        ASSERT(infolog);
        glGetShaderInfoLog(shader, loglen, NULL, infolog);
        ASSERT(!glGetError());
        infolog[loglen] = 0;
        return infolog;
    }
    return NULL;
}

static char* GetProgramLog(glhandle program)
{
    ASSERT(program);
    i32 status = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    ASSERT(!glGetError());
    if (!status)
    {
        i32 loglen = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &loglen);
        ASSERT(!glGetError());
        char* infolog = pim_calloc(EAlloc_Temp, loglen + 1);
        glGetProgramInfoLog(program, loglen, NULL, infolog);
        ASSERT(!glGetError());
        infolog[loglen] = 0;
        return infolog;
    }
    return NULL;
}

static glhandle CreateGlProgram(
    i32 vsLines, const char* const * const vs,
    i32 fsLines, const char* const * const fs)
{
    ASSERT(vsLines > 0);
    ASSERT(fsLines > 0);
    ASSERT(vs);
    ASSERT(fs);

    glhandle vso = glCreateShader(GL_VERTEX_SHADER);
    ASSERT(vso);
    ASSERT(!glGetError());
    glShaderSource(vso, vsLines, vs, NULL);
    ASSERT(!glGetError());
    glCompileShader(vso);
    ASSERT(!glGetError());
    char* vsErrors = GetShaderLog(vso);
    if (vsErrors)
    {
        fd_puts(vsErrors, fd_stderr);
        pim_free(vsErrors);
        glDeleteShader(vso);
        ASSERT(!glGetError());
        return 0;
    }

    glhandle fso = glCreateShader(GL_FRAGMENT_SHADER);
    ASSERT(fso);
    ASSERT(!glGetError());
    glShaderSource(fso, fsLines, fs, NULL);
    ASSERT(!glGetError());
    glCompileShader(fso);
    ASSERT(!glGetError());
    char* fsErrors = GetShaderLog(vso);
    if (fsErrors)
    {
        fd_puts(fsErrors, fd_stderr);
        pim_free(fsErrors);
        glDeleteShader(vso);
        ASSERT(!glGetError());
        glDeleteShader(fso);
        ASSERT(!glGetError());
        return 0;
    }

    glhandle prog = glCreateProgram();
    ASSERT(prog);
    ASSERT(!glGetError());
    glAttachShader(prog, vso);
    ASSERT(!glGetError());
    glAttachShader(prog, fso);
    ASSERT(!glGetError());
    glLinkProgram(prog);
    ASSERT(!glGetError());

    char* progErrors = GetProgramLog(prog);
    if (progErrors)
    {
        fd_puts(progErrors, fd_stderr);
        pim_free(progErrors);
        glDeleteProgram(prog);
        ASSERT(!glGetError());
        prog = 0;
    }

    glDeleteShader(vso);
    ASSERT(!glGetError());
    glDeleteShader(fso);
    ASSERT(!glGetError());

    return prog;
}

static void DestroyGlProgram(glhandle* pProg)
{
    ASSERT(pProg);
    glhandle prog = *pProg;
    *pProg = 0;
    if (prog)
    {
        glUseProgram(0);
        ASSERT(!glGetError());
        glDeleteProgram(prog);
        ASSERT(!glGetError());
    }
}

static void SetupTextureUnit(glhandle prog, const char* texName, i32 unit)
{
    ASSERT(prog);
    ASSERT(texName);
    i32 location = glGetUniformLocation(prog, texName);
    ASSERT(!glGetError());
    ASSERT(location != -1);
    glUseProgram(prog);
    ASSERT(!glGetError());
    glUniform1i(location, unit);
    ASSERT(!glGetError());
}
