#include "math/cubic_fit.h"
#include "math/scalar.h"
#include "math/float3_funcs.h"
#include "common/random.h"
#include "threading/task.h"

typedef float(VEC_CALL *ErrFn)(dataset_t data, fit_t fit);

static float VEC_CALL CubicError(dataset_t data, fit_t fit);
static float VEC_CALL SqrticError(dataset_t data, fit_t fit);
static float VEC_CALL TMapError(dataset_t data, fit_t fit);

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
    task_t task;
    ErrFn errFn;
    dataset_t data;
    i32 iterations;
    float errors[kNumThreads];
    fit_t fits[kNumThreads];
} FitTask;

static float VEC_CALL randf32(prng_t* rng)
{
    return 2.0f * prng_f32(rng) - 1.0f;
}

static void VEC_CALL randFit(prng_t* rng, fit_t* fit)
{
    float* pim_noalias value = fit->value;
    for (i32 i = 0; i < NELEM(fit->value); ++i)
    {
        value[i] = randf32(rng);
    }
}

static void VEC_CALL mutateFit(prng_t* rng, fit_t* fit, float eps)
{
    float* pim_noalias value = fit->value;
    for (i32 i = 0; i < NELEM(fit->value); ++i)
    {
        value[i] += eps * randf32(rng);
    }
}

static void FitFn(task_t* task, i32 begin, i32 end)
{
    FitTask* fitTask = (FitTask*)task;
    const dataset_t data = fitTask->data;
    const i32 iterations = i1_max(fitTask->iterations,  2 * (data.len + 1));
    const ErrFn errFn = fitTask->errFn;
    prng_t rng = prng_create();

    for (i32 eval = begin; eval < end; ++eval)
    {
        fit_t fit;
        randFit(&rng, &fit);
        float error = errFn(data, fit);

        for (i32 j = 0; j < iterations; ++j)
        {
            float eps = 10.0f;
            for (i32 i = 0; i < 12; ++i)
            {
                eps *= 0.5f;
                fit_t testFit = fit;
                mutateFit(&rng, &testFit, eps);
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
}

static float CreateFit(
    dataset_t data,
    fit_t* fit,
    i32 iterations,
    FitType type)
{
    FitTask task = { 0 };
    task.errFn = ms_errFns[type];
    task.data = data;
    task.iterations = iterations;
    task_submit(&task.task, FitFn, NELEM(task.fits));
    task_sys_schedule();
    task_await(&task.task);
    i32 chosen = 0;
    for (i32 i = 1; i < NELEM(task.errors); ++i)
    {
        if (task.errors[i] < task.errors[chosen])
        {
            chosen = i;
        }
    }
    *fit = task.fits[chosen];
    return task.errors[chosen];
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

// ----------------------------------------------------------------------------
// factoring out the eval functions as a function pointer
// would introduce call overhead into the innermost loop.
// instead we factor out the loop itself.

static float VEC_CALL CubicError(dataset_t data, fit_t fit)
{
    const float weight = 1.0f / data.len;
    float error = 0.0f;
    for (i32 i = 0; i < data.len; ++i)
    {
        float y = CubicEval(data.xs[i], fit);
        float diff = y - data.ys[i];
        error += weight * (diff * diff);
    }
    return sqrtf(error);
}

static float VEC_CALL SqrticError(dataset_t data, fit_t fit)
{
    const float weight = 1.0f / data.len;
    float error = 0.0f;
    for (i32 i = 0; i < data.len; ++i)
    {
        float y = SqrticEval(data.xs[i], fit);
        float diff = y - data.ys[i];
        error += weight * (diff * diff);
    }
    return sqrtf(error);
}

static float VEC_CALL TMapError(dataset_t data, fit_t fit)
{
    const float weight = 1.0f / data.len;
    float error = 0.0f;
    for (i32 i = 0; i < data.len; ++i)
    {
        float y = TMapEval(data.xs[i], fit);
        float diff = y - data.ys[i];
        error += weight * (diff * diff);
    }
    return sqrtf(error);
}
