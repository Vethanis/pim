#include "rendering/transform_compose.h"

#include "components/ecs.h"
#include "common/profiler.h"
#include "math/float4x4_funcs.h"
#include "allocator/allocator.h"

static void TransformComposeForEach(ecs_foreach_t* task, void** rows, i32 length)
{
    ASSERT(rows);
    const position_t* pim_noalias positions = rows[CompId_Position];
    const rotation_t* pim_noalias rotations = rows[CompId_Rotation];
    const scale_t* pim_noalias scales = rows[CompId_Scale];
    localtoworld_t* pim_noalias matrices = rows[CompId_LocalToWorld];
    ASSERT(matrices);

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

static const compflag_t kAll = { .dwords[0] = (1 << CompId_LocalToWorld) };
static const compflag_t kNone = { 0 };

ProfileMark(pm_TransformCompose, TransformCompose)
struct task_s* TransformCompose(void)
{
    ProfileBegin(pm_TransformCompose);

    ecs_foreach_t* task = tmp_calloc(sizeof(*task));
    ecs_foreach(task, kAll, kNone, TransformComposeForEach);

    ProfileEnd(pm_TransformCompose);
    return (task_t*)task;
}
