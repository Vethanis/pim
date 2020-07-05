#include "rendering/exposure.h"
#include "rendering/mipmap.h"
#include "rendering/sampler.h"
#include "math/color.h"
#include "threading/task.h"
#include "allocator/allocator.h"
#include "common/profiler.h"

typedef struct task_ToLum
{
    task_t task;
    const float4* light;
    float averages[kNumThreads];
    int2 size;
} task_ToLum;

static float AverageLum(const float4* pim_noalias light, i32 length)
{
    const float weight = 1.0f / length;
    float average = 0.0f;
    for (i32 i = 0; i < length; ++i)
    {
        average += weight * f4_perlum(light[i]);
    }
    return average;
}

static void ToLumFn(task_t* pbase, i32 begin, i32 end)
{
    task_ToLum* task = (task_ToLum*)pbase;
    const float4* pim_noalias light = task->light;
    float* pim_noalias averages = task->averages;
    const i32 len = task->size.x * task->size.y;
    const i32 lenPerJob = len / kNumThreads;

    for (i32 i = begin; i < end; ++i)
    {
        averages[i] = AverageLum(light + i * lenPerJob, lenPerJob);
    }
}

ProfileMark(pm_toluminance, ToLuminance)
static float ToLuminance(int2 size, const float4* light)
{
    ProfileBegin(pm_toluminance);
    task_ToLum* task = tmp_calloc(sizeof(*task));
    task->light = light;
    task->size = size;
    task_run(&task->task, ToLumFn, kNumThreads);
    const float weight = 1.0f / kNumThreads;
    float avgLum = 0.0f;
    for (i32 i = 0; i < kNumThreads; ++i)
    {
        avgLum += weight * task->averages[i];
    }
    ProfileEnd(pm_toluminance);
    return avgLum;
}

typedef struct task_Expose
{
    task_t task;
    float4* light;
    float exposure;
} task_Expose;

static void ExposeFn(task_t* pbase, i32 begin, i32 end)
{
    task_Expose* task = (task_Expose*)pbase;
    float4* pim_noalias light = task->light;
    const float exposure = task->exposure;

    for (i32 i = begin; i < end; ++i)
    {
        light[i] = f4_mulvs(light[i], exposure);
    }
}

ProfileMark(pm_avglum, AverageLum)
ProfileMark(pm_exposeimg, ExposeImage)
void ExposeImage(
    int2 size,
    float4* pim_noalias light,
    exposure_t* parameters)
{
    float avgLum = ToLuminance(size, light);
    avgLum = AdaptLuminance(
        parameters->avgLum,
        avgLum,
        parameters->deltaTime,
        parameters->adaptRate);
    parameters->avgLum = avgLum;
    float exposure = CalcExposure(*parameters);

    ProfileBegin(pm_exposeimg);

    task_Expose* task = tmp_calloc(sizeof(*task));
    task->light = light;
    task->exposure = exposure;
    task_run(&task->task, ExposeFn, size.x * size.y);

    ProfileEnd(pm_exposeimg);
}
