#include "rendering/exposure.h"
#include "rendering/mipmap.h"
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
    const float4* light;
    float* averages;
} task_ToLum;

static void CalcAverageFn(Task* pbase, i32 begin, i32 end)
{
    task_ToLum* task = (task_ToLum*)pbase;
    float* pim_noalias averages = task->averages;
    const float4* pim_noalias light = task->light;
    const int2 size = task->size;
    const float weight = 1.0f / size.x;
    for (i32 i = begin; i < end; ++i)
    {
        const float4* pim_noalias row = &light[size.x * i];
        float sum = 0.0f;
        for (i32 j = 0; j < size.x; ++j)
        {
            sum += f4_avglum(row[j]) * weight;
        }
        averages[i] = sum;
    }
}

static float CalcAverage(const float4* light, int2 size)
{
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

ProfileMark(pm_exposeimg, ExposeImage)
void ExposeImage(
    int2 size,
    float4* pim_noalias light,
    vkrExposure* parameters)
{
    ProfileBegin(pm_exposeimg);

    parameters->deltaTime = f1_lerp(parameters->deltaTime, (float)Time_Deltaf(), 0.25f);

    parameters->avgLum = AdaptLuminance(
        parameters->avgLum,
        CalcAverage(light, size),
        parameters->deltaTime,
        parameters->adaptRate);
    parameters->exposure = CalcExposure(parameters);

    task_Expose* task = Temp_Calloc(sizeof(*task));
    task->light = light;
    task->exposure = parameters->exposure;
    Task_Run(&task->task, ExposeFn, size.x * size.y);

    ProfileEnd(pm_exposeimg);
}
