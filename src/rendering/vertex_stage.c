#include "rendering/vertex_stage.h"

#include "allocator/allocator.h"
#include "common/atomics.h"
#include "common/profiler.h"
#include "components/table.h"
#include "components/components.h"
#include "components/drawables.h"
#include "threading/task.h"

#include "math/types.h"
#include "math/float2_funcs.h"
#include "math/float3_funcs.h"
#include "math/float4_funcs.h"
#include "math/float4x4_funcs.h"
#include "math/quat_funcs.h"
#include "math/frustum.h"

#include "rendering/mesh.h"
#include "rendering/camera.h"
#include "rendering/constants.h"
#include "rendering/framebuffer.h"

static u64 ms_entsCulled;
static u64 ms_entsDrawn;

u64 Vert_EntsCulled(void) { return ms_entsCulled; }
u64 Vert_EntsDrawn(void) { return ms_entsDrawn; }

typedef struct verttask_s
{
    task_t task;
    frus_t frus;
    drawable_t* drawables;
    const localtoworld_t* matrices;
} verttask_t;

pim_inline float2 VEC_CALL TransformUv(float2 uv, float4 ST)
{
    uv.x = uv.x * ST.x + ST.z;
    uv.y = uv.y * ST.y + ST.w;
    return uv;
}

static meshid_t VEC_CALL TransformMesh(frus_t frus, meshid_t local, float4x4 M, float4 ST)
{
    meshid_t result = { 0 };

    mesh_t mesh;
    if (mesh_get(local, &mesh) && mesh.length > 0)
    {
        i32 vertCount = 0;
        float4* pim_noalias positions = tmp_malloc(sizeof(positions[0]) * mesh.length);
        float4* pim_noalias normals = tmp_malloc(sizeof(normals[0]) * mesh.length);
        float2* pim_noalias uvs = tmp_malloc(sizeof(uvs[0]) * mesh.length);

        const float4x4 IM = f4x4_inverse(f4x4_transpose(M)); // TODO: use f3x3, faster inverse
        for (i32 j = 0; (j + 3) <= mesh.length; j += 3)
        {
            float4 A = f4x4_mul_pt(M, mesh.positions[j + 0]);
            float4 B = f4x4_mul_pt(M, mesh.positions[j + 1]);
            float4 C = f4x4_mul_pt(M, mesh.positions[j + 2]);
            sphere_t sph = triToSphere(A, B, C);
            float dist = sdFrusSph(frus, sph);
            if (dist <= 0.0f)
            {
                i32 a = vertCount++;
                i32 b = vertCount++;
                i32 c = vertCount++;

                positions[a] = A;
                positions[b] = B;
                positions[c] = C;

                normals[a] = f4_normalize3(f4x4_mul_dir(IM, mesh.normals[j + 0]));
                normals[b] = f4_normalize3(f4x4_mul_dir(IM, mesh.normals[j + 1]));
                normals[c] = f4_normalize3(f4x4_mul_dir(IM, mesh.normals[j + 2]));

                uvs[a] = TransformUv(mesh.uvs[j + 0], ST);
                uvs[b] = TransformUv(mesh.uvs[j + 1], ST);
                uvs[c] = TransformUv(mesh.uvs[j + 2], ST);
            }
        }

        if (vertCount > 0)
        {
            mesh_t* tmpmesh = tmp_calloc(sizeof(*tmpmesh));
            tmpmesh->length = vertCount;
            tmpmesh->positions = positions;
            tmpmesh->normals = normals;
            tmpmesh->uvs = uvs;
            ASSERT(tmpmesh->version == 0);
            ASSERT(tmpmesh->length == vertCount);
            ASSERT(tmpmesh->positions == positions);
            ASSERT(tmpmesh->normals == normals);
            ASSERT(tmpmesh->uvs == uvs);
            result = mesh_create(tmpmesh);
        }
    }
    else
    {
        // shouldn't happen unless a visible mesh is destroyed mid-draw.
        // isn't fatal, just want to catch cases where this occurs.
        // might want to assert that the mesh is occluded or very far away
        ASSERT(false);
    }

    return result;
}

static void VertexFn(task_t* pBase, i32 begin, i32 end)
{
    verttask_t* pTask = (verttask_t*)pBase;
    drawable_t* pim_noalias drawables = pTask->drawables;
    const localtoworld_t* pim_noalias matrices = pTask->matrices;

    for (i32 i = begin; i < end; ++i)
    {
        drawables[i].tmpmesh.handle = NULL;
        drawables[i].tmpmesh.version = 0;

        if (!drawables[i].tilemask)
        {
            continue;
        }

        meshid_t tmpmesh = TransformMesh(
            pTask->frus,
            drawables[i].mesh,
            matrices[i].Value,
            drawables[i].material.st);
        drawables[i].tmpmesh = tmpmesh;
        if (!tmpmesh.handle)
        {
            drawables[i].tilemask = 0;
        }
    }
}

ProfileMark(pm_Vertex, Drawables_Vertex)
task_t* Drawables_Vertex(struct tables_s* tables, const struct camera_s* camera)
{
    ProfileBegin(pm_Vertex);
    ASSERT(camera);
    task_t* result = NULL;

    table_t* table = Drawables_Get(tables);
    if (table)
    {
        verttask_t* task = tmp_calloc(sizeof(*task));
        camera_frustum(camera, &(task->frus));
        task->drawables = table_row(table, TYPE_ARGS(drawable_t));
        task->matrices = table_row(table, TYPE_ARGS(localtoworld_t));
        task_submit((task_t*)task, VertexFn, table_width(table));
        result = (task_t*)task;
    }

    ProfileEnd(pm_Vertex);
    return result;
}
