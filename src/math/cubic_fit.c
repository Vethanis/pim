#include "math/cubic_fit.h"
#include "math/scalar.h"
#include "math/float3_funcs.h"
#include "common/random.h"
#include "threading/task.h"
#include "allocator/allocator.h"

static float VEC_CALL CubicError(float3 fit, const float* pim_noalias ys, i32 len)
{
    const float dx = 1.0f / len;
    float error = 0.0f;
    float x = 0.0f;
    for (i32 i = 0; i < len; ++i)
    {
        float y = CubicEval(fit, x);
        if (y < 0.0f || y > 1.0f)
        {
            return 1000.0f;
        }
        float diff = y - ys[i];
        error += dx * (diff * diff);
        x += dx;
    }
    return sqrtf(error);
}

static float VEC_CALL SqrticError(float3 fit, const float* pim_noalias ys, i32 len)
{
    const float dx = 1.0f / len;
    float error = 0.0f;
    float x = 0.0f;
    for (i32 i = 0; i < len; ++i)
    {
        float y = SqrticEval(fit, x);
        if (y < 0.0f || y > 1.0f)
        {
            return 1000.0f;
        }
        float diff = y - ys[i];
        error += dx * (diff * diff);
        x += dx;
    }
    return sqrtf(error);
}

static float VEC_CALL randf32(prng_t* rng)
{
    return 2.0f * prng_f32(rng) - 1.0f;
}

static float3 VEC_CALL randf3(prng_t* rng)
{
    float3 y = { randf32(rng), randf32(rng), randf32(rng) };
    return y;
}

typedef struct FitTask
{
    task_t task;
    const float* pim_noalias ys;
    i32 len;
    i32 iterations;
    float3 fits[256];
    float errors[256];
} FitTask;

static void CubicFitFn(task_t* task, i32 begin, i32 end)
{
    FitTask* fitTask = (FitTask*)task;
    const float* pim_noalias ys = fitTask->ys;
    const i32 len = fitTask->len;
    const i32 iterations = fitTask->iterations;

    for (i32 eval = begin; eval < end; ++eval)
    {
        prng_t rng = prng_create();
        float3 fit = randf3(&rng);
        float error = CubicError(fit, ys, len);
        for (i32 j = 0; j < iterations; ++j)
        {
            for (i32 i = 0; i < 10; ++i)
            {
                float eps = 1.0f / (1 << i);
                float3 f2 = f3_add(fit, f3_mulvs(randf3(&rng), eps));
                float e = CubicError(f2, ys, len);
                if (e < error)
                {
                    error = e;
                    fit = f2;
                }
            }
        }
        fitTask->fits[eval] = fit;
        fitTask->errors[eval] = error;
    }
}

static void SqrticFitFn(task_t* task, i32 begin, i32 end)
{
    FitTask* fitTask = (FitTask*)task;
    const float* pim_noalias ys = fitTask->ys;
    const i32 len = fitTask->len;
    const i32 iterations = fitTask->iterations;

    for (i32 eval = begin; eval < end; ++eval)
    {
        prng_t rng = prng_create();
        float3 fit = randf3(&rng);
        float error = SqrticError(fit, ys, len);
        for (i32 j = 0; j < iterations; ++j)
        {
            for (i32 i = 0; i < 10; ++i)
            {
                float eps = 1.0f / (1 << i);
                float3 f2 = f3_add(fit, f3_mulvs(randf3(&rng), eps));
                float e = SqrticError(f2, ys, len);
                if (e < error)
                {
                    error = e;
                    fit = f2;
                }
            }
        }
        fitTask->fits[eval] = fit;
        fitTask->errors[eval] = error;
    }
}

float3 CubicFit(
    const float* pim_noalias ys,
    i32 len,
    i32 iterations,
    float* pim_noalias fitError)
{
    FitTask task = { 0 };
    task.ys = ys;
    task.len = len;
    task.iterations = iterations;
    task_submit(&task.task, CubicFitFn, NELEM(task.fits));
    task_sys_schedule();
    task_await(&task.task);
    i32 chosen = 0;
    for (i32 i = 1; i < NELEM(task.fits); ++i)
    {
        if (task.errors[i] < task.errors[chosen])
        {
            chosen = i;
        }
    }
    *fitError = task.errors[chosen];
    return task.fits[chosen];
}

float3 SqrticFit(
    const float* pim_noalias ys,
    i32 len,
    i32 iterations,
    float* pim_noalias fitError)
{
    FitTask task = { 0 };
    task.ys = ys;
    task.len = len;
    task.iterations = iterations;
    task_submit(&task.task, SqrticFitFn, NELEM(task.fits));
    task_sys_schedule();
    task_await(&task.task);
    i32 chosen = 0;
    for (i32 i = 1; i < NELEM(task.fits); ++i)
    {
        if (task.errors[i] < task.errors[chosen])
        {
            chosen = i;
        }
    }
    *fitError = task.errors[chosen];
    return task.fits[chosen];
}
