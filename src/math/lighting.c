#include "math/lighting.h"

#include "math/sampling.h"
#include "rendering/cubemap.h"
#include "rendering/sampler.h"
#include "rendering/texture.h"

#include "allocator/allocator.h"
#include "common/console.h"
#include "common/cvars.h"
#include "common/profiler.h"
#include "threading/task.h"

#include <stb/stb_image_write.h>
#include <string.h>

pim_optimize

BrdfLut g_BrdfLut;
static TextureId ms_brdfTexId;
static i32 ms_brdfLutSamples;

typedef struct BakeBrdfTask
{
    Task task;
    BrdfLut lut;
    i32 prevSamples;
    i32 numSamples;
} BakeBrdfTask;

static void BakeBrdfFn(void* pbase, i32 begin, i32 end)
{
    BakeBrdfTask* pim_noalias task = pbase;
    float2* pim_noalias texels = task->lut.texels;
    const int2 size = task->lut.size;
    const i32 prevSamples = task->prevSamples;
    const i32 numSamples = task->numSamples;
    const i32 deltaSamples = numSamples - prevSamples;
    const float dx = 1.0f / deltaSamples;
    const float dt = (float)deltaSamples / (float)numSamples;
    const float rcpWidth = 1.0f / size.x;
    const float rcpHeight = 1.0f / size.y;

    Prng* rng = Prng_Get();
    for (i32 iTexel = begin; iTexel < end; ++iTexel)
    {
        const int2 coord = EncodeCoord2(size, iTexel);

        float2 result = f2_0;
        float sum = 0.0f;
        for (i32 iSample = prevSamples; iSample < numSamples; ++iSample)
        {
            float2 aa = SampleGaussPixelFilter(f2_rand(rng), 0.5f);
            const float NoV = f1_clamp((coord.x + aa.x) * rcpWidth, kEpsilon, 1.0f - kEpsilon);
            const float alpha = f1_clamp((coord.y + aa.y) * rcpHeight, kMinAlpha, 1.0f);
            const float4 V = SphericalToCartesian(NoV, Prng_f32(rng) * kTau);
            ASSERT(IsUnitLength(V));

            // N = (0, 0, 1)
            float4 H = SampleGGXMicrofacet(f2_rand(rng), alpha);
            ASSERT(IsUnitLength(H));
            ASSERT(H.z >= 0.0f);

            float4 L = f4_reflect3(f4_neg(V), H);
            ASSERT(IsUnitLength(L));

            float NoL = L.z;
            float NoH = H.z;
            float HoV = f4_dot3(H, V);
            float pdf = GGXPdf(NoH, HoV, alpha);

            if ((NoL > kEpsilon) && (pdf > kEpsilon))
            {
                float D = D_GTR(NoH, alpha) / pdf;
                float G = V_SmithCorrelated(NoL, NoV, alpha);
                float Fc = F_Dielectric(HoV, 1.000293f, 1.52f);
                float DG_NoL_dx = D * G * NoL * dx;
                result.x += DG_NoL_dx * Fc;
                result.y += DG_NoL_dx;
            }
        }
        texels[iTexel] = f2_lerpvs(texels[iTexel], result, dt);
    }
}

BrdfLut BrdfLut_New(int2 size, i32 numSamples)
{
    ASSERT(size.x > 0);
    ASSERT(size.y > 0);

    BakeBrdfTask* task = Temp_Calloc(sizeof(*task));
    task->lut.texels = Tex_Calloc(sizeof(task->lut.texels[0]) * size.x * size.y);
    task->lut.size = size;
    task->prevSamples = 0;
    task->numSamples = numSamples;
    Task_Run(task, BakeBrdfFn, size.x * size.y);

    return task->lut;
}

void BrdfLut_Del(BrdfLut* lut)
{
    Mem_Free(lut->texels);
    memset(lut, 0, sizeof(*lut));
}

void BrdfLut_Update(BrdfLut* lut, i32 prevSamples, i32 newSamples)
{
    ASSERT(lut->texels);
    if (lut->texels && (newSamples > prevSamples))
    {
        BakeBrdfTask* task = Temp_Calloc(sizeof(*task));
        task->lut = *lut;
        task->prevSamples = prevSamples;
        task->numSamples = newSamples;
        Task_Run(task, BakeBrdfFn, lut->size.x * lut->size.y);
    }
}

static void SaveLutImage(BrdfLut lut, const char* path)
{
    const i32 texelCount = lut.size.x * lut.size.y;
    R8G8B8A8_t* pim_noalias ldrtexels = Tex_Calloc(sizeof(ldrtexels[0]) * texelCount);
    for (i32 i = 0; i < texelCount; ++i)
    {
        float4 v = { lut.texels[i].x, lut.texels[i].y, 0.0f, 1.0f };
        ldrtexels[i] = GammaEncode_rgba8(v);
    }
    stbi_write_png(path, lut.size.x, lut.size.y, 4, ldrtexels, sizeof(ldrtexels[0]) * lut.size.x);
    Mem_Free(ldrtexels);
}

void LightingSys_Init(void)
{
    const i32 lutSize = 128 DEBUG_ONLY(/4);
    const i32 numSamples = 256 DEBUG_ONLY(/4);
    g_BrdfLut = BrdfLut_New(i2_s(lutSize), numSamples);

    Texture tex = { 0 };
    tex.format = VK_FORMAT_R32G32_SFLOAT;
    tex.size = g_BrdfLut.size;
    tex.texels = g_BrdfLut.texels;
    Texture_New(&tex, tex.format, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, Guid_FromStr("g_BrdfLut"), &ms_brdfTexId);
}

ProfileMark(pm_update, LightingSys_Update)
void LightingSys_Update(void)
{
    ProfileBegin(pm_update);

    i32 samplesPerFrame = ConVar_GetInt(&cv_r_brdflut_spf);
    if (samplesPerFrame > 0)
    {
        BrdfLut_Update(&g_BrdfLut, ms_brdfLutSamples, ms_brdfLutSamples + samplesPerFrame);
        ms_brdfLutSamples += samplesPerFrame;
        Texture_Upload(ms_brdfTexId);
    }

    ProfileEnd(pm_update);
}

void LightingSys_Shutdown(void)
{
    Texture_Release(ms_brdfTexId);
    memset(&g_BrdfLut, 0, sizeof(g_BrdfLut));
}
