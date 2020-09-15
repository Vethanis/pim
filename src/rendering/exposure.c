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
    i32 lenPerJob;
    float averages[kMaxThreads];
} task_ToLum;

static void CalcAverageFn(task_t* pbase, i32 begin, i32 end)
{
    task_ToLum* task = (task_ToLum*)pbase;
    float* pim_noalias averages = task->averages;
    const float4* pim_noalias light = task->light;
    const i32 lenPerJob = task->lenPerJob;
    const float weight = 1.0f / lenPerJob;
    for (i32 i = begin; i < end; ++i)
    {
        const float4* pim_noalias jobLight = light + lenPerJob * i;
        float sum = 0.0f;
        for (i32 j = 0; j < lenPerJob; ++j)
        {
            float lum = f4_perlum(jobLight[j]);
            sum = weight * lum + sum;
        }
        averages[i] = sum;
    }
}

static float CalcAverage(const float4* light, int2 size)
{
    const i32 numthreads = task_thread_ct();
    const i32 lenPerJob = (size.x * size.y) / numthreads;
    task_ToLum* task = tmp_calloc(sizeof(*task));
    task->light = light;
    task->lenPerJob = lenPerJob;
    task_run(&task->task, CalcAverageFn, numthreads);
    const float* pim_noalias averages = task->averages;
    float sum = 0.0f;
    const float weight = 1.0f / numthreads;
    for (i32 i = 0; i < numthreads; ++i)
    {
        sum = weight * averages[i] + sum;
    }
    return sum;
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

ProfileMark(pm_exposeimg, ExposeImage)
void ExposeImage(
    int2 size,
    float4* pim_noalias light,
    vkrExposure* parameters)
{
    ProfileBegin(pm_exposeimg);

    parameters->avgLum = AdaptLuminance(
        parameters->avgLum,
        CalcAverage(light, size),
        parameters->deltaTime,
        parameters->adaptRate);
    parameters->exposure = CalcExposure(parameters);

    task_Expose* task = tmp_calloc(sizeof(*task));
    task->light = light;
    task->exposure = parameters->exposure;
    task_run(&task->task, ExposeFn, size.x * size.y);

    ProfileEnd(pm_exposeimg);
}
