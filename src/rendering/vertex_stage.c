#include "rendering/vertex_stage.h"

#include "allocator/allocator.h"
#include "common/atomics.h"
#include "common/profiler.h"
#include "components/ecs.h"

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
#include "rendering/fragment_stage.h"
#include "rendering/sampler.h"

typedef struct vertexstage_s
{
    ecs_foreach_t task;
    framebuf_t* target;
} vertexstage_t;

static u64 ms_entsCulled;
static u64 ms_entsDrawn;

u64 Vert_EntsCulled(void) { return ms_entsCulled; }
u64 Vert_EntsDrawn(void) { return ms_entsDrawn; }

pim_optimize
static void VertexStageForEach(ecs_foreach_t* task, void** rows, const i32* indices, i32 length)
{
    vertexstage_t* vertTask = (vertexstage_t*)task;
    ASSERT(vertTask->target);

    const float4x4* pim_noalias matrices = rows[CompId_LocalToWorld];
    const bounds_t* pim_noalias bounds = rows[CompId_Bounds];
    drawable_t* pim_noalias drawables = rows[CompId_Drawable];

    camera_t camera;
    camera_get(&camera);
    frus_t frus;
    camera_frustum(&camera, &frus);

    // frustum-mesh culling
    for (i32 i = 0; i < length; ++i)
    {
        const i32 e = indices[i];
        const box_t box = TransformBox(matrices[e], bounds[e].box);
        const bool visible = sdFrusBoxTest(frus, box) <= 0.0f;
        drawables[e].visible = visible;

#if CULLING_STATS
        if (visible)
        {
            inc_u64(&ms_entsDrawn, MO_Relaxed);
        }
        else
        {
            inc_u64(&ms_entsCulled, MO_Relaxed);
        }
#endif // CULLING_STATS
    }

    // vertex transform + frustum-triangle culling
    for (i32 i = 0; i < length; ++i)
    {
        const i32 e = indices[i];
        if (!drawables[e].visible)
        {
            continue;
        }

        mesh_t meshIn = { 0 };
        if (!mesh_get(drawables[e].mesh, &meshIn))
        {
            drawables[e].visible = false;
            continue;
        }

        // TODO: float3x3, inverse, transpose, and mul_dir.
        const float4x4 M = matrices[e];
        const float4x4 IM = f4x4_inverse(f4x4_transpose(M));
        const float4 ST = drawables[e].material.st;
        const i32 numVerts = meshIn.length;

        const float4* pim_noalias positionsIn = meshIn.positions;
        const float4* pim_noalias normalsIn = meshIn.normals;
        const float2* pim_noalias uvsIn = meshIn.uvs;

        // hopefully this doesn't blow up the linear allocator.
        // if it does, grow the hell out of it and shrink the persistent allocator.
        float4* pim_noalias positionsOut = tmp_malloc(sizeof(positionsOut[0]) * numVerts);
        float4* pim_noalias normalsOut = tmp_malloc(sizeof(normalsOut[0]) * numVerts);
        float2* pim_noalias uvsOut = tmp_malloc(sizeof(uvsOut[0]) * numVerts);

        for (i32 j = 0; j < numVerts; ++j)
        {
            positionsOut[j] = f4x4_mul_pt(M, positionsIn[j]);
        }
        for (i32 j = 0; j < numVerts; ++j)
        {
            normalsOut[j] = f4_normalize3(f4x4_mul_dir(IM, normalsIn[j]));
        }
        for(i32 j = 0; j < numVerts; ++j)
        {
            uvsOut[j] = TransformUv(uvsIn[j], ST);
        }

        mesh_t* meshOut = tmp_calloc(sizeof(*meshOut));
        meshOut->length = numVerts;
        meshOut->positions = positionsOut;
        meshOut->normals = normalsOut;
        meshOut->uvs = uvsOut;
        meshid_t worldMesh = mesh_create(meshOut);

        SubmitMesh(worldMesh, drawables[e].material);
    }
}

static const compflag_t kAll =
{
    .dwords[0] =
        (1 << CompId_Drawable) |
        (1 << CompId_Bounds) |
        (1 << CompId_LocalToWorld)
};
static const compflag_t kNone = { 0 };

ProfileMark(pm_VertexStage, VertexStage)
task_t* VertexStage(struct framebuf_s* target)
{
    ProfileBegin(pm_VertexStage);

#if CULLING_STATS
    store_u64(&ms_entsCulled, 0, MO_Release);
    store_u64(&ms_entsDrawn, 0, MO_Release);
    store_u64(&ms_trisCulled, 0, MO_Release);
    store_u64(&ms_trisDrawn, 0, MO_Release);
#endif // CULLING_STATS

    vertexstage_t* task = tmp_calloc(sizeof(*task));
    task->target = target;
    ecs_foreach((ecs_foreach_t*)task, kAll, kNone, VertexStageForEach);

    ProfileEnd(pm_VertexStage);
    return (task_t*)task;
}
