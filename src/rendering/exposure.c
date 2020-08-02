#include "rendering/exposure.h"
#include "rendering/mipmap.h"
#include "rendering/sampler.h"
#include "math/color.h"
#include "threading/task.h"
#include "allocator/allocator.h"
#include "common/profiler.h"

// ----------------------------------------------------------------------------
// Legend
// ----------------------------------------------------------------------------
// N: relative aperture, in f-stops
// t: Shutter time, in seconds
// S: Sensor sensitivity, in ISO
// q: Proportion of light traveling through lens to sensor

// ----------------------------------------------------------------------------
// Constants
// ----------------------------------------------------------------------------
// K: 12.5, standard camera calibration
// q: 0.65, with theta=10deg, T=0.9, v=0.98

// ----------------------------------------------------------------------------
// Equations
// ----------------------------------------------------------------------------
// q = (pi/4) * T * v(theta) * pow(cos(theta), 4)
//
// EV100 = log2(NN/t) when S = 100
//       = log2(NN/t * 100/S)
//       = log2((Lavg * S) / K)

static float ManualEV100(
    float aperture,
    float shutterTime,
    float ISO)
{
    // EVs = log2(NN/t)
    // EV100 = EVs - log2(S/100)
    // log(a) - log(b) = log(a * 1/b)
    float a = (aperture * aperture) / shutterTime;
    float b = 100.0f / ISO;
    return log2f(a * b);
}

static float LumToEV100(float Lavg)
{
    // EV100 = log2((Lavg * S) / K)
    // S = 100
    // K = 12.5
    // S/K = 8
    return log2f(Lavg * 8.0f);
}

// https://www.desmos.com/calculator/vorr8hwdl7
static float EV100ToLum(float ev100)
{
    return exp2f(ev100) / 8.0f;
}

static float SaturationExposure(float ev100)
{
    // The factor 78 is chosen such that exposure settings based on a standard
    // light meter and an 18-percent reflective surface will result in an image
    // with a saturation of 12.7%
    // EV100 = log2(NN/t)
    // Lmax = (78/(Sq)) * (NN/t)
    // Lmax = (78/(100 * 0.65)) * 2^EV100
    // Lmax = 1.2 * 2^EV100
    const float factor = 78.0f / (100.0f * 0.65f);
    float Lmax = factor * exp2f(ev100);
    return 1.0f / Lmax;
}

static float StandardExposure(float ev100)
{
    const float midGrey = 0.18f;
    const float factor = 10.0f / (100.0f * 0.65f);
    float Lavg = factor * exp2f(ev100);
    return midGrey / Lavg;
}

static float CalcExposure(exposure_t args)
{
    float ev100 = 0.0f;
    if (args.manual)
    {
        ev100 = ManualEV100(args.aperture, args.shutterTime, args.ISO);
    }
    else
    {
        ev100 = LumToEV100(args.avgLum);
    }

    ev100 = f1_max(ev100, 0.0f);
    ev100 = ev100 - args.offsetEV;

    float exposure;
    if (args.standard)
    {
        exposure = StandardExposure(ev100);
    }
    else
    {
        exposure = SaturationExposure(ev100);
    }
    return exposure;
}

static float AdaptLuminance(
    float lum0,
    float lum1,
    float dt,
    float rate)
{
    float t = 1.0f - expf(-dt * rate);
    return f1_lerp(lum0, lum1, f1_sat(t));
}

typedef struct task_ToLum
{
    task_t task;
    const float4* light;
    i32 lenPerJob;
    float averages[kNumThreads];
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
    const i32 lenPerJob = (size.x * size.y) / kNumThreads;
    task_ToLum* task = tmp_calloc(sizeof(*task));
    task->light = light;
    task->lenPerJob = lenPerJob;
    task_run(&task->task, CalcAverageFn, kNumThreads);
    const float* pim_noalias averages = task->averages;
    float sum = 0.0f;
    const float weight = 1.0f / kNumThreads;
    for (i32 i = 0; i < kNumThreads; ++i)
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
    exposure_t* parameters)
{
    ProfileBegin(pm_exposeimg);

    float avgLum = CalcAverage(light, size);
    avgLum = AdaptLuminance(
        parameters->avgLum,
        avgLum,
        parameters->deltaTime,
        parameters->adaptRate);
    parameters->avgLum = avgLum;
    float exposure = CalcExposure(*parameters);

    task_Expose* task = tmp_calloc(sizeof(*task));
    task->light = light;
    task->exposure = exposure;
    task_run(&task->task, ExposeFn, size.x * size.y);

    ProfileEnd(pm_exposeimg);
}
