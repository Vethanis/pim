#include "rendering/exposure.h"
#include "rendering/sampler.h"
#include "math/color.h"
#include "threading/task.h"
#include "allocator/allocator.h"
#include "common/profiler.h"
#include "common/time.h"
#include "common/cvars.h"

#include <float.h>

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
    const int2 size = task->size;
    const float4* pim_noalias light = task->light;
    float* pim_noalias averages = task->averages;

    const float weight = 1.0f / size.x;
    for (i32 y = begin; y < end; ++y)
    {
        const i32 i0 = size.x * y;
        const i32 i1 = size.x * (y + 1);
        float average = 0.0f;
        for (i32 i = i0; i < i1; ++i)
        {
            float lum = f4_avglum(light[i]);
            average += lum * weight;
        }
        averages[y] = average;
    }
}

ProfileMark(pm_average, CalcAverage)
static float CalcAverage(const float4* light, int2 size)
{
    ProfileBegin(pm_average);

    const i32 worksize = size.y;
    task_ToLum* task = Temp_Calloc(sizeof(*task));
    task->size = size;
    task->light = light;
    task->averages = Temp_Alloc(sizeof(task->averages[0]) * worksize);
    Task_Run(&task->task, CalcAverageFn, worksize);

    const float* pim_noalias averages = task->averages;
    const float weight = 1.0f / worksize;
    float avgLum = 0.0f;
    for (i32 i = 0; i < worksize; ++i)
    {
        avgLum += averages[i] * weight;
    }

    ProfileEnd(pm_average);
    return avgLum;
}

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

    ProfileEnd(pm_exposeimg);
}
