#include "rendering/transform_compose.h"

#include "components/ecs.h"
#include "common/profiler.h"
#include "math/float4x4_funcs.h"
#include "allocator/allocator.h"

static void Compose_Id(ecs_foreach_t* task, void** rows, const i32* indices, i32 length)
{
    if (length > 0)
    {
        float4x4* pim_noalias matrices = rows[CompId_LocalToWorld];
        const float4x4 M = f4x4_id;
        for (i32 i = 0; i < length; ++i)
        {
            const i32 e = indices[i];
            matrices[e] = M;
        }
    }
}

static void Compose_T(ecs_foreach_t* task, void** rows, const i32* indices, i32 length)
{
    if (length > 0)
    {
        float4x4* pim_noalias matrices = rows[CompId_LocalToWorld];
        const translation_t* pim_noalias translations = rows[CompId_Translation];
        for (i32 i = 0; i < length; ++i)
        {
            const i32 e = indices[i];
            matrices[e] = f4x4_translation(translations[e].Value);
        }
    }
}

static void Compose_R(ecs_foreach_t* task, void** rows, const i32* indices, i32 length)
{
    if (length > 0)
    {
        float4x4* pim_noalias matrices = rows[CompId_LocalToWorld];
        const float3 T = f3_0;
        const rotation_t* pim_noalias rotations = rows[CompId_Rotation];
        const float3 S = f3_1;
        for (i32 i = 0; i < length; ++i)
        {
            const i32 e = indices[i];
            matrices[e] = f4x4_trs(T, rotations[e].Value, S);
        }
    }
}

static void Compose_S(ecs_foreach_t* task, void** rows, const i32* indices, i32 length)
{
    if (length > 0)
    {
        float4x4* pim_noalias matrices = rows[CompId_LocalToWorld];
        const float3 T = f3_0;
        const quat R = quat_id;
        const scale_t* pim_noalias scales = rows[CompId_Scale];
        for (i32 i = 0; i < length; ++i)
        {
            const i32 e = indices[i];
            matrices[e] = f4x4_trs(T, R, scales[e].Value);
        }
    }
}

static void Compose_TR(ecs_foreach_t* task, void** rows, const i32* indices, i32 length)
{
    if (length > 0)
    {
        float4x4* pim_noalias matrices = rows[CompId_LocalToWorld];
        const translation_t* pim_noalias translations = rows[CompId_Translation];
        const rotation_t* pim_noalias rotations = rows[CompId_Rotation];
        const float3 S = f3_1;
        for (i32 i = 0; i < length; ++i)
        {
            const i32 e = indices[i];
            matrices[e] = f4x4_trs(translations[e].Value, rotations[e].Value, S);
        }
    }
}

static void Compose_TS(ecs_foreach_t* task, void** rows, const i32* indices, i32 length)
{
    if (length > 0)
    {
        float4x4* pim_noalias matrices = rows[CompId_LocalToWorld];
        const translation_t* pim_noalias translations = rows[CompId_Translation];
        const quat R = quat_id;
        const scale_t* pim_noalias scales = rows[CompId_Scale];
        for (i32 i = 0; i < length; ++i)
        {
            const i32 e = indices[i];
            matrices[e] = f4x4_trs(translations[e].Value, R, scales[e].Value);
        }
    }
}

static void Compose_RS(ecs_foreach_t* task, void** rows, const i32* indices, i32 length)
{
    if (length > 0)
    {
        float4x4* pim_noalias matrices = rows[CompId_LocalToWorld];
        const float3 T = f3_0;
        const rotation_t* pim_noalias rotations = rows[CompId_Rotation];
        const scale_t* pim_noalias scales = rows[CompId_Scale];
        for (i32 i = 0; i < length; ++i)
        {
            const i32 e = indices[i];
            matrices[e] = f4x4_trs(T, rotations[e].Value, scales[e].Value);
        }
    }
}

static void Compose_TRS(ecs_foreach_t* task, void** rows, const i32* indices, i32 length)
{
    if (length > 0)
    {
        float4x4* pim_noalias matrices = rows[CompId_LocalToWorld];
        const translation_t* pim_noalias translations = rows[CompId_Translation];
        const rotation_t* pim_noalias rotations = rows[CompId_Rotation];
        const scale_t* pim_noalias scales = rows[CompId_Scale];
        for (i32 i = 0; i < length; ++i)
        {
            const i32 e = indices[i];
            matrices[e] = f4x4_trs(translations[e].Value, rotations[e].Value, scales[e].Value);
        }
    }
}

ProfileMark(pm_TransformCompose, TransformCompose)
struct task_s* TransformCompose(void)
{
    ProfileBegin(pm_TransformCompose);

    ecs_foreach_t* task = tmp_calloc(sizeof(*task));
    ecs_foreach(task,
        compflag_create(1, CompId_LocalToWorld),
        compflag_create(3, CompId_Translation, CompId_Rotation, CompId_Scale),
        Compose_Id);

    task = tmp_calloc(sizeof(*task));
    ecs_foreach(task,
        compflag_create(2, CompId_LocalToWorld, CompId_Translation),
        compflag_create(2, CompId_Rotation, CompId_Scale),
        Compose_T);

    task = tmp_calloc(sizeof(*task));
    ecs_foreach(task,
        compflag_create(2, CompId_LocalToWorld, CompId_Rotation),
        compflag_create(2, CompId_Translation, CompId_Scale),
        Compose_R);

    task = tmp_calloc(sizeof(*task));
    ecs_foreach(task,
        compflag_create(2, CompId_LocalToWorld, CompId_Scale),
        compflag_create(2, CompId_Translation, CompId_Rotation),
        Compose_S);

    task = tmp_calloc(sizeof(*task));
    ecs_foreach(task,
        compflag_create(3, CompId_LocalToWorld, CompId_Translation, CompId_Rotation),
        compflag_create(1, CompId_Scale),
        Compose_TR);

    task = tmp_calloc(sizeof(*task));
    ecs_foreach(task,
        compflag_create(3, CompId_LocalToWorld, CompId_Translation, CompId_Scale),
        compflag_create(1, CompId_Rotation),
        Compose_TS);

    task = tmp_calloc(sizeof(*task));
    ecs_foreach(task,
        compflag_create(4, CompId_LocalToWorld, CompId_Translation, CompId_Rotation, CompId_Scale),
        compflag_create(0),
        Compose_TRS);

    ProfileEnd(pm_TransformCompose);
    return (task_t*)task;
}
