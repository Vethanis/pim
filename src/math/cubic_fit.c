#include "math/cubic_fit.h"
#include "math/scalar.h"
#include "math/float3_funcs.h"
#include "common/random.h"
#include "threading/task.h"
#include "allocator/allocator.h"

typedef float(*ErrFn)(dataset_t data, fit_t fit);

pim_inline float CubicError(dataset_t data, fit_t fit)
{
    float error = 0.0f;
    for (i32 i = 0; i < data.len; ++i)
    {
        float y = CubicEval(data.xs[i], fit);
        float diff = y - data.ys[i];
        error = f1_max(error, diff * diff);
    }
    return sqrtf(error);
}

pim_inline float SqrticError(dataset_t data, fit_t fit)
{
    float error = 0.0f;
    for (i32 i = 0; i < data.len; ++i)
    {
        float y = SqrticEval(data.xs[i], fit);
        float diff = y - data.ys[i];
        error = f1_max(error, diff * diff);
    }
    return sqrtf(error);
}

pim_inline float TMapError(dataset_t data, fit_t fit)
{
    float error = 0.0f;
    for (i32 i = 0; i < data.len; ++i)
    {
        float y = TMapEval(data.xs[i], fit);
        float diff = y - data.ys[i];
        error = f1_max(error, diff * diff);
    }
    return sqrtf(error);
}

typedef enum
{
    Fit_Cubic,
    Fit_Sqrtic,
    Fit_TMap,
} FitType;

static const ErrFn ms_errFns[] =
{
    CubicError,
    SqrticError,
    TMapError,
};

typedef struct FitTask
{
    Task task;
    ErrFn errFn;
    dataset_t data;
    i32 iterations;
    float errors[kMaxThreads];
    fit_t fits[kMaxThreads];
} FitTask;

pim_inline float randf32(Prng* rng)
{
    return 2.0f * Prng_f32(rng) - 1.0f;
}

pim_inline void randFit(Prng* rng, fit_t* fit)
{
    float* pim_noalias value = fit->value;
    for (i32 i = 0; i < NELEM(fit->value); ++i)
    {
        value[i] = randf32(rng);
    }
}

pim_inline void mutateFit(Prng* rng, fit_t* fit, float amt)
{
    float* pim_noalias value = fit->value;
    for (i32 i = 0; i < NELEM(fit->value); ++i)
    {
        value[i] += amt * randf32(rng);
    }
}

static void FitFn(void* pbase, i32 begin, i32 end)
{
    FitTask* fitTask = pbase;
    const dataset_t data = fitTask->data;
    const i32 iterations = i1_max(fitTask->iterations, 2 * (data.len + 1));
    const ErrFn errFn = fitTask->errFn;

    Prng rng = Prng_Get();
    for (i32 eval = begin; eval < end; ++eval)
    {
        fit_t fit;
        randFit(&rng, &fit);
        float error = errFn(data, fit);

        for (i32 j = 0; j < iterations; ++j)
        {
            for (i32 i = 0; i < 22; ++i)
            {
                fit_t testFit = fit;
                mutateFit(&rng, &testFit, 1.0f / (1 << i));
                float testErr = errFn(data, testFit);
                if (testErr < error)
                {
                    error = testErr;
                    fit = testFit;
                }
            }
        }

        fitTask->fits[eval] = fit;
        fitTask->errors[eval] = error;
    }
    Prng_Set(rng);
}

static float CreateFit(
    dataset_t data,
    fit_t* fit,
    i32 iterations,
    FitType type)
{
    const i32 numthreads = Task_ThreadCount();
    FitTask* task = Temp_Calloc(sizeof(*task));
    task->errFn = ms_errFns[type];
    task->data = data;
    task->iterations = iterations;
    Task_Run(task, FitFn, numthreads);

    i32 chosen = 0;
    float chosenError = 1 << 20;
    const float* pim_noalias errors = task->errors;
    for (i32 i = 1; i < numthreads; ++i)
    {
        float error = errors[i];
        if (error < chosenError)
        {
            chosen = i;
            chosenError = error;
        }
    }

    *fit = task->fits[chosen];
    return chosenError;
}

float CubicFit(dataset_t data, fit_t* fit, i32 iterations)
{
    return CreateFit(data, fit, iterations, Fit_Cubic);
}

float SqrticFit(dataset_t data, fit_t* fit, i32 iterations)
{
    return CreateFit(data, fit, iterations, Fit_Sqrtic);
}

float TMapFit(dataset_t data, fit_t* fit, i32 iterations)
{
    return CreateFit(data, fit, iterations, Fit_TMap);
}
