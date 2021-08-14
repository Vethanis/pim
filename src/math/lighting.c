#include "math/lighting.h"
#include "math/sampling.h"
#include "rendering/sampler.h"
#include "allocator/allocator.h"
#include "threading/task.h"
#include "rendering/cubemap.h"
#include <string.h>

BrdfLut g_BrdfLut;

typedef struct BakeBrdfTask
{
    Task task;
    BrdfLut lut;
    u32 numSamples;
} BakeBrdfTask;

static void BakeBrdfFn(void* pbase, i32 begin, i32 end)
{
    BakeBrdfTask* pim_noalias task = pbase;
    float2* pim_noalias texels = task->lut.texels;
    const int2 size = task->lut.size;
    const float2 dUv = { 1.0f / size.x, 1.0f / size.y };
    const u32 numSamples = task->numSamples;
    const float dx = 1.0f / numSamples;

    Prng rng = Prng_Get();
    float2 Xi = f2_rand(&rng);
    for (i32 iTexel = begin; iTexel < end; ++iTexel)
    {
        const float2 coord = i2_f2(IndexToCoord(size, iTexel));

        float2 result = { 0.0f, 0.0f };
        for (u32 iSample = 0; iSample < numSamples; ++iSample)
        {
            const float2 uv = f2_mul(f2_add(coord, f2_rand(&rng)), dUv);
            const float NoV = uv.x;
            const float alpha = f1_max(uv.y, kMinAlpha);
            const float4 V = SphericalToCartesian(NoV, 0.0f);
            ASSERT(IsUnitLength(V));

            Xi = f2_frac(f2_add(Xi, f2_v(kGoldenConj, kSqrt2Conj)));
            // N = (0, 0, 1)
            float4 H = SampleGGXMicrofacet(Xi, alpha);
            ASSERT(IsUnitLength(H));
            ASSERT(H.z >= 0.0f);

            float4 L = f4_reflect3(f4_neg(V), H);
            ASSERT(IsUnitLength(L));
            L.z = f1_abs(L.z);

            float NoL = f1_sat(L.z);
            float NoH = f1_sat(H.z);
            float HoV = f4_dotsat(H, V);
            float pdf = GGXPdf(NoH, HoV, alpha);

            if ((NoL > 0.0f) && (pdf > 0.0f))
            {
                float D = D_GTR(NoH, alpha) / pdf;
                float G = V_SmithCorrelated(NoL, NoV, alpha);
                float Fc = F_Schlick1(0.0f, 1.0f, HoV);
                float DG_NoL_dx = D * G * NoL * dx;
                result.x += DG_NoL_dx * Fc;
                result.y += DG_NoL_dx;
            }
        }

        texels[iTexel] = result;
    }

    Prng_Set(rng);
}

BrdfLut BrdfLut_New(int2 size, u32 numSamples)
{
    ASSERT(size.x > 0);
    ASSERT(size.y > 0);

    BakeBrdfTask* task = Temp_Calloc(sizeof(*task));
    task->lut.texels = Tex_Alloc(sizeof(task->lut.texels[0]) * size.x * size.y);
    task->lut.size = size;
    task->numSamples = numSamples;
    Task_Run(task, BakeBrdfFn, size.x * size.y);

    return task->lut;
}

void BrdfLut_Del(BrdfLut* lut)
{
    Mem_Free(lut->texels);
    memset(lut, 0, sizeof(*lut));
}
