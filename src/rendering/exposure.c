#include "rendering/exposure.h"
#include "rendering/sampler.h"
#include "math/color.h"
#include "threading/task.h"
#include "allocator/allocator.h"
#include "common/profiler.h"
#include "common/time.h"

typedef struct task_ToLum
{
    Task task;
    int2 size;
    const float4* pim_noalias light;
    float* pim_noalias averages;
} task_ToLum;

static void CalcAverageFn(void* pbase, i32 begin, i32 end)
{
    task_ToLum* task = pbase;
    float* pim_noalias averages = task->averages;
    const float4* pim_noalias light = task->light;
    const int2 size = task->size;
    const i32 kSamples = size.x;
    const float weight = 1.0f / kSamples;
    Prng rng = Prng_Get();
    float2 Xi = f2_rand(&rng);
    for (i32 i = begin; i < end; ++i)
    {
        float sum = 0.0f;
        for (i32 j = 0; j < kSamples; ++j)
        {
            Xi.x = f1_wrap(Xi.x + kGoldenConj);
            Xi.y = f1_wrap(Xi.y + kSqrt2Conj);
            float angle = Xi.x * kTau;
            float radius = f1_gauss_invcdf(Xi.y, 0.0f, 0.25f);
            float2 uv = f2_v(cosf(angle), sinf(angle));
            uv = f2_mulvs(uv, radius);
            uv = f2_unorm(uv);
            i32 iTexel = UvClamp(size, uv);
            sum += f4_perlum(light[iTexel]) * weight;
        }
        averages[i] = sum;
    }
    Prng_Set(rng);
}

ProfileMark(pm_average, CalcAverage)
static float CalcAverage(const float4* light, int2 size)
{
    ProfileBegin(pm_average);

    task_ToLum* task = Temp_Calloc(sizeof(*task));
    task->light = light;
    task->size = size;
    task->averages = Temp_Calloc(sizeof(task->averages[0]) * size.y);
    Task_Run(&task->task, CalcAverageFn, size.y);

    const float* pim_noalias averages = task->averages;
    const float weight = 1.0f / size.y;
    float sum = 0.0f;
    for (i32 i = 0; i < size.y; ++i)
    {
        sum += averages[i] * weight;
    }

    ProfileEnd(pm_average);
    return sum;
}

typedef struct task_Expose
{
    Task task;
    float4* light;
    float exposure;
} task_Expose;

static void ExposeFn(Task* pbase, i32 begin, i32 end)
{
    task_Expose* task = (task_Expose*)pbase;
    float4* pim_noalias light = task->light;
    const float exposure = task->exposure;

    for (i32 i = begin; i < end; ++i)
    {
        light[i] = f4_mulvs(light[i], exposure);
    }
}

ProfileMark(pm_apply, ApplyExposure)
ProfileMark(pm_exposeimg, ExposeImage)
void ExposeImage(
    int2 size,
    float4* pim_noalias light,
    vkrExposure* parameters)
{
    ProfileBegin(pm_exposeimg);

    parameters->deltaTime = f1_lerp(parameters->deltaTime, (float)Time_Deltaf(), 0.5f);

    parameters->avgLum = AdaptLuminance(
        parameters->avgLum,
        CalcAverage(light, size),
        parameters->deltaTime,
        parameters->adaptRate);
    parameters->exposure = CalcExposure(parameters);

    ProfileBegin(pm_apply);
    task_Expose* task = Temp_Calloc(sizeof(*task));
    task->light = light;
    task->exposure = parameters->exposure;
    Task_Run(&task->task, ExposeFn, size.x * size.y);
    ProfileEnd(pm_apply);

    ProfileEnd(pm_exposeimg);
}
