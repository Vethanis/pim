#include "rendering/path_tracer.h"
#include "rendering/material.h"
#include "rendering/mesh.h"
#include "rendering/camera.h"

#include "math/float3_funcs.h"
#include "math/float4_funcs.h"
#include "math/quat_funcs.h"
#include "math/float4x4_funcs.h"
#include "math/lighting.h"
#include "math/sampling.h"
#include "math/sdf.h"
#include "math/frustum.h"

#include "allocator/allocator.h"
#include "components/ecs.h"
#include "threading/task.h"
#include "common/random.h"

typedef struct pt_node_s
{
    box_t bounds;
    //  <0: null
    // >=0: child index
    struct pt_node_s* children[8];
    i32* faces; // points to first vertex of a triangle in the scene
    i32 faceCount;
} pt_node_t;

typedef struct pt_scene_s
{
    float4 min;
    float4 max;

    float4* lightPos;
    float4* lightDir;
    // xyz: radiance of light, HDR value in [0, 1024]
    // w  > 0: spherical light radius
    // w == 0: directional light
    // w  < 0: spotlight angle * -1
    float4* lightRad;

    // all meshes within the scene
    // xyz: vertex position
    //   w: 1
    float4* positions;
    // xyz: vertex normal
    //   w: material index
    float4* normals;
    //  xy: texture coordinate
    float2* uvs;

    // surface description, indexed by normal.w
    material_t* materials;

    // bounding volume hierarchy
    struct pt_node_s* root;

    i32 lightCount;
    i32 vertCount;
    i32 matCount;
} pt_scene_t;

static bool VEC_CALL node_contains(box_t box, float4 A, float4 B, float4 C)
{
    return
        (sdBox3D(box.center, box.extents, A) <= 0.0f) &&
        (sdBox3D(box.center, box.extents, B) <= 0.0f) &&
        (sdBox3D(box.center, box.extents, C) <= 0.0f);
}

static void VEC_CALL node_subdivide(box_t bounds, pt_node_t** children)
{
    for (i32 i = 0; i < 8; ++i)
    {
        pt_node_t* child = children[i];
        if (!child)
        {
            child = perm_calloc(sizeof(pt_node_t));
            float4 sign =
            {
                (i & 1) ? -1.0f : 1.0f,
                (i & 2) ? -1.0f : 1.0f,
                (i & 4) ? -1.0f : 1.0f,
                1.0f,
            };
            box_t subBounds;
            subBounds.extents = f4_mulvs(bounds.extents, 0.5f);
            subBounds.center = f4_add(bounds.center, f4_mul(subBounds.extents, sign));
            child->bounds = subBounds;
            children[i] = child;
        }
    }
}

static void VEC_CALL InsertTri(pt_node_t* node, i32 iVert, float4 A, float4 B, float4 C)
{
    box_t bounds = node->bounds;
    if (node_contains(bounds, A, B, C))
    {
        pt_node_t** children = node->children;
        node_subdivide(bounds, children);
        bool insideChild = false;
        for (i32 i = 0; i < 8; ++i)
        {
            pt_node_t* child = children[i];
            if (node_contains(child->bounds, A, B, C))
            {
                InsertTri(child, iVert, A, B, C);
                insideChild = true;
                break;
            }
        }
        if (!insideChild)
        {
            const i32 faceCount = 1 + node->faceCount;
            node->faceCount = faceCount;
            node->faces = perm_realloc(node->faces, sizeof(node->faces[0]) * faceCount);
            node->faces[faceCount - 1] = iVert;
        }
    }
}

struct pt_scene_s* pt_scene_new(void)
{
    pt_scene_t* pim_noalias scene = perm_calloc(sizeof(*scene));

    const u32 lightQuery =
        (1 << CompId_Light) |
        (1 << CompId_Translation) |
        (1 << CompId_Rotation);

    i32 lightCount = 0;
    ent_t* pim_noalias lightEnts = ecs_query(lightQuery, 0, &lightCount);
    float4* pim_noalias lightPos = perm_malloc(sizeof(lightPos[0]) * lightCount);
    float4* pim_noalias lightDir = perm_malloc(sizeof(lightDir[0]) * lightCount);
    float4* pim_noalias lightRad = perm_malloc(sizeof(lightRad[0]) * lightCount);
    for (i32 i = 0; i < lightCount; ++i)
    {
        const ent_t ent = lightEnts[i];
        const light_t* pLight = ecs_get(ent, CompId_Light);
        const translation_t* pT = ecs_get(ent, CompId_Translation);
        const rotation_t* pR = ecs_get(ent, CompId_Rotation);

        lightRad[i] = pLight->radiance;
        lightDir[i] = f3_f4(quat_fwd(pR->Value), 0.0f);
        lightPos[i] = f3_f4(pT->Value, 1.0f);
    }
    scene->lightCount = lightCount;
    scene->lightPos = lightPos;
    scene->lightDir = lightDir;
    scene->lightRad = lightRad;

    const u32 meshQuery =
        (1 << CompId_Drawable) |
        (1 << CompId_LocalToWorld);

    i32 meshCount = 0;
    i32 vertCount = 0;
    i32 matCount = 0;
    ent_t* pim_noalias meshEnts = ecs_query(meshQuery, 0, &meshCount);
    for (i32 i = 0; i < meshCount; ++i)
    {
        const ent_t ent = meshEnts[i];
        const drawable_t* drawable = ecs_get(ent, CompId_Drawable);
        mesh_t mesh;
        if (mesh_get(drawable->mesh, &mesh))
        {
            vertCount += mesh.length;
            ++matCount;
        }
    }

    float4* pim_noalias positions = perm_malloc(sizeof(positions[0]) * vertCount);
    float4* pim_noalias normals = perm_malloc(sizeof(normals[0]) * vertCount);
    float2* pim_noalias uvs = perm_malloc(sizeof(uvs[0]) * vertCount);
    material_t* pim_noalias materials = perm_malloc(sizeof(materials[0]) * matCount);

    const float kBigNum = 1 << 20;
    float4 sceneMin = f4_s(kBigNum);
    float4 sceneMax = f4_s(-kBigNum);
    i32 vertBack = 0;
    i32 matBack = 0;
    for (i32 i = 0; i < meshCount; ++i)
    {
        const ent_t ent = meshEnts[i];
        const drawable_t* drawable = ecs_get(ent, CompId_Drawable);

        mesh_t mesh;
        if (mesh_get(drawable->mesh, &mesh))
        {
            const localtoworld_t* l2w = ecs_get(ent, CompId_LocalToWorld);
            const float4x4 M = l2w->Value;
            const float4x4 IM = f4x4_inverse(f4x4_transpose(M));
            const material_t material = drawable->material;

            for (i32 j = 0; j < mesh.length; ++j)
            {
                positions[vertBack + j] = f4x4_mul_pt(M, mesh.positions[j]);
            }

            for (i32 j = 0; j < mesh.length; ++j)
            {
                const float4 position = positions[vertBack + j];
                sceneMin = f4_min(sceneMin, position);
                sceneMax = f4_max(sceneMax, position);
            }

            for (i32 j = 0; j < mesh.length; ++j)
            {
                normals[vertBack + j] = f4x4_mul_dir(IM, mesh.positions[j]);
                normals[vertBack + j].w = (float)matBack;
            }

            for (i32 j = 0; j < mesh.length; ++j)
            {
                float2 uv = mesh.uvs[j];
                uv.x = uv.x * material.st.x + material.st.z;
                uv.y = uv.y * material.st.y + material.st.w;
                uvs[vertBack + j] = uv;
            }

            vertBack += mesh.length;
            materials[matBack] = material;
            ++matBack;
        }
    }

    ASSERT(vertBack == vertCount);
    ASSERT(matBack == matCount);

    scene->vertCount = vertCount;
    scene->positions = positions;
    scene->normals = normals;
    scene->uvs = uvs;

    scene->matCount = matCount;
    scene->materials = materials;

    sceneMax = f4_addvs(sceneMax, 0.01f);
    sceneMin = f4_addvs(sceneMin, -0.01f);
    box_t bounds;
    bounds.center = f4_lerp(sceneMin, sceneMax, 0.5f);
    bounds.extents = f4_sub(sceneMax, bounds.center);

    pt_node_t* root = perm_calloc(sizeof(*root) * 64);
    root->bounds = bounds;

    for (i32 i = 0; (i + 3) <= vertCount; i += 3)
    {
        InsertTri(root, i, positions[i], positions[i + 1], positions[i + 2]);
    }
}

static void node_del(pt_node_t* node)
{
    if (node)
    {
        pt_node_t** children = node->children;
        for (i32 i = 0; i < 8; ++i)
        {
            node_del(children[i]);
        }
        pim_free(node->faces);
        memset(node, 0, sizeof(*node));
        pim_free(node);
    }
}

void pt_scene_del(struct pt_scene_s* scene)
{
    ASSERT(scene);

    pim_free(scene->lightDir);
    pim_free(scene->lightPos);
    pim_free(scene->lightRad);

    pim_free(scene->positions);
    pim_free(scene->normals);
    pim_free(scene->uvs);

    pim_free(scene->materials);
    node_del(scene->root);

    memset(scene, 0, sizeof(*scene));
    pim_free(scene);
}

typedef struct trace_task_s
{
    task_t task;
    const pt_scene_t* scene;
    camera_t camera;
    float4* image;
    int2 imageSize;
    i32 spp;
    i32 bounces;
} trace_task_t;

static float4 VEC_CALL TraceRay(
    const float4* pim_noalias vertices,
    const pt_node_t* pim_noalias node,
    float4 ro,
    float4 rd,
    float4 rcpRd,
    float t,        // nearest triangle traced thus far
    i32* indexOut)
{
    float4 wuvt = f4_s(-1.0f);
    i32 index = -1;
    {
        float2 tBox = isectBox3D(ro, rcpRd, node->bounds);
        if (tBox.x < 0.0f || tBox.x >= tBox.y || tBox.x > t)
        {
            *indexOut = index;
            return wuvt;
        }
    }
    {
        const i32 len = node->faceCount;
        const i32* pim_noalias faces = node->faces;
        for (i32 i = 0; i < len; ++i)
        {
            const i32 v = faces[i];
            float4 tri = isectTri3D(
                ro,
                rd,
                vertices[v + 0],
                vertices[v + 1],
                vertices[v + 2]);
            if (tri.w < t && f4_all(f4_gteqvs(tri, 0.0f)))
            {
                wuvt = tri;
                t = tri.w;
                index = v;
            }
        }
    }
    {
        const pt_node_t* const* pim_noalias children = node->children;
        for (i32 i = 0; i < 8; ++i)
        {
            if (children[i])
            {
                i32 childIndex = -1;
                float4 child = TraceRay(
                    vertices,
                    children[i],
                    ro,
                    rd,
                    rcpRd,
                    t,
                    &childIndex);
                if (childIndex >= 0 && child.w < t)
                {
                    wuvt = child;
                    t = child.w;
                    index = childIndex;
                }
            }
        }
    }
    *indexOut = index;
    return wuvt;
}

static float4 VEC_CALL TracePixel(
    const pt_scene_t* pim_noalias scene,
    float4 ro,
    float4 rd,
    i32 spp,
    i32 bounces)
{
    // const float4 rcpRd = f4_rcp(rd);

    return f4_s(1.0f);
}

static void TraceFn(task_t* task, i32 begin, i32 end)
{
    trace_task_t* trace = (trace_task_t*)task;

    const pt_scene_t* pim_noalias scene = trace->scene;
    float4* pim_noalias image = trace->image;
    const int2 size = trace->imageSize;
    const i32 spp = trace->spp;
    const i32 bounces = trace->bounces;
    const camera_t camera = trace->camera;
    const quat rot = camera.rotation;
    const float4 ro = f3_f4(camera.position, 1.0f);
    const float3 right = quat_right(rot);
    const float3 up = quat_up(rot);
    const float3 fwd = quat_fwd(rot);
    const float2 slope = proj_slope(f1_radians(camera.fovy), (float)size.x / (float)size.y);

    const float dx = 1.0f / size.x;
    const float dy = 1.0f / size.y;
    for (i32 i = begin; i < end; ++i)
    {
        const i32 y = i / size.x;
        const i32 x = i % size.x;
        const float2 coord = f2_snorm(f2_v(x * dx, y * dy));
        const float4 rd = f3_f4(proj_dir(right, up, fwd, slope, coord), 1.0f);
        image[i] = TracePixel(scene, ro, rd, spp, bounces);
    }
}

struct task_s* pt_trace(pt_trace_t* desc)
{
    ASSERT(desc);
    ASSERT(desc->scene);
    ASSERT(desc->dstImage);
    ASSERT(desc->camera);

    trace_task_t* task = tmp_calloc(sizeof(*task));
    task->scene = desc->scene;
    task->camera = desc->camera[0];
    task->image = desc->dstImage;
    task->imageSize = desc->imageSize;
    task->spp = desc->samplesPerPixel;
    task->bounces = desc->bounces;

    const i32 workSize = desc->imageSize.x * desc->imageSize.y;
    task_submit((task_t*)task, TraceFn, workSize);
    return (task_t*)task;
}
