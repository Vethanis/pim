#include "math/cubic_fit.h"
#include "math/scalar.h"
#include "math/float3_funcs.h"
#include "common/random.h"
#include "threading/task.h"
#include "allocator/allocator.h"

typedef float(VEC_CALL *ErrFn)(dataset_t data, fit_t fit);

pim_inline float VEC_CALL CubicError(dataset_t data, fit_t fit)
{
    float err = 0.0f;
    const float dx = 1.0f / data.len;
    for (i32 i = 0; i < data.len; ++i)
    {
        float y = CubicEval(data.xs[i], fit);
        float diff = y - data.ys[i];
        err += diff * diff * dx;
    }
    return sqrtf(err);
}

pim_inline float VEC_CALL SqrticError(dataset_t data, fit_t fit)
{
    float err = 0.0f;
    const float dx = 1.0f / data.len;
    for (i32 i = 0; i < data.len; ++i)
    {
        float y = SqrticEval(data.xs[i], fit);
        float diff = y - data.ys[i];
        err += diff * diff * dx;
    }
    return sqrtf(err);
}

pim_inline float VEC_CALL TMapError(dataset_t data, fit_t fit)
{
    float err = 0.0f;
    const float dx = 1.0f / data.len;
    for (i32 i = 0; i < data.len; ++i)
    {
        float y = TMapEval(data.xs[i], fit);
        float diff = y - data.ys[i];
        err += diff * diff * dx;
    }
    return sqrtf(err);
}

pim_inline float VEC_CALL PolyError(dataset_t data, fit_t fit)
{
    float err = 0.0f;
    const float dx = 1.0f / data.len;
    for (i32 i = 0; i < data.len; ++i)
    {
        float y = PolyEval(data.xs[i], fit);
        float diff = y - data.ys[i];
        err += diff * diff * dx;
    }
    return sqrtf(err);
}

typedef enum
{
    Fit_Cubic,
    Fit_Sqrtic,
    Fit_TMap,
    Fit_Poly,

    Fit_COUNT
} FitType;

static const ErrFn ms_errFns[] =
{
    CubicError,
    SqrticError,
    TMapError,
    PolyError,
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
    for (i32 i = 0; i < NELEM(fit->value); ++i)
    {
        fit->value[i] = randf32(rng);
    }
}

pim_inline void mutateFit(Prng* rng, fit_t* fit, float amt)
{
    for (i32 i = 0; i < NELEM(fit->value); ++i)
    {
        fit->value[i] += amt * randf32(rng);
    }
}

static void FitFn(void* pbase, i32 begin, i32 end)
{
    FitTask* fitTask = pbase;
    const dataset_t data = fitTask->data;
    const i32 iterations = i1_max(fitTask->iterations, 2 * (data.len + 1));
    const ErrFn errFn = fitTask->errFn;

    Prng rng = Prng_Get();
    for (i32 iFit = begin; iFit < end; ++iFit)
    {
        fit_t fit;
        randFit(&rng, &fit);
        float error = errFn(data, fit);

        for (i32 iIter = 0; iIter < iterations; ++iIter)
        {
            for (i32 iBit = 0; iBit < 22; ++iBit)
            {
                fit_t testFit = fit;
                mutateFit(&rng, &testFit, 1.0f / (1 << iBit));
                float testErr = errFn(data, testFit);
                if (testErr < error)
                {
                    error = testErr;
                    fit = testFit;
                }
            }
        }

        fitTask->fits[iFit] = fit;
        fitTask->errors[iFit] = error;
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

float PolyFit(dataset_t data, fit_t* fit, i32 iterations)
{
    return CreateFit(data, fit, iterations, Fit_Poly);
}

