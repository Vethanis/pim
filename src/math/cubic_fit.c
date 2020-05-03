#include "math/cubic_fit.h"
#include "math/scalar.h"
#include "math/float3_funcs.h"
#include "common/random.h"
#include "threading/task.h"

static float VEC_CALL CubicError(float3 fit, const float* pim_noalias ys, i32 len)
{
    const float dx = 1.0f / len;
    float error = 0.0f;
    float x = 0.0f;
    for (i32 i = 0; i < len; ++i)
    {
        float y = CubicEval(fit, x);
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

typedef float(VEC_CALL *ErrFn)(float3 fit, const float* pim_noalias ys, i32 len);

typedef struct FitTask
{
    task_t task;
    ErrFn errFn;
    const float* pim_noalias ys;
    i32 len;
    float3 fits[kNumThreads];
    float errors[kNumThreads];
} FitTask;

static void FitFn(task_t* task, i32 begin, i32 end)
{
    FitTask* fitTask = (FitTask*)task;
    const float* pim_noalias ys = fitTask->ys;
    const i32 len = fitTask->len;
    const i32 iterations = 2 * (len + 1);
    const ErrFn errFn = fitTask->errFn;
    prng_t rng = prng_create();

    for (i32 eval = begin; eval < end; ++eval)
    {
        float3 fit = randf3(&rng);
        float error = errFn(fit, ys, len);

        for (i32 j = 0; j < iterations; ++j)
        {
            float eps = 1.0f;
            for (i32 i = 0; i < 12; ++i)
            {
                float3 f2 = f3_add(fit, f3_mulvs(randf3(&rng), eps));
                float e = errFn(f2, ys, len);
                if (e < error)
                {
                    error = e;
                    fit = f2;
                }
                eps *= 0.5f;
            }
        }

        fitTask->fits[eval] = fit;
        fitTask->errors[eval] = error;
    }
}

typedef enum
{
    Fit_Cubic,
    Fit_Sqrtic,
} FitType;

static const ErrFn ms_errFns[] =
{
    CubicError,
    SqrticError,
};

static float3 CreateFit(
    const float* pim_noalias ys,
    i32 len,
    float* pim_noalias fitError,
    FitType type)
{
    FitTask task = { 0 };
    task.errFn = ms_errFns[type];
    task.ys = ys;
    task.len = len;
    task_submit(&task.task, FitFn, NELEM(task.fits));
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

float3 CubicFit(
    const float* pim_noalias ys,
    i32 len,
    float* pim_noalias fitError)
{
    return CreateFit(ys, len, fitError, Fit_Cubic);
}

float3 SqrticFit(
    const float* pim_noalias ys,
    i32 len,
    float* pim_noalias fitError)
{
    return CreateFit(ys, len, fitError, Fit_Sqrtic);
}
