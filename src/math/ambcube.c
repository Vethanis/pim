#include "math/ambcube.h"
#include "rendering/path_tracer.h"
#include "math/float3_funcs.h"

i32 AmbCube_Bake(
    pt_scene_t* scene,
    AmbCube_t* pCube,
    float4 origin,
    i32 samples,
    i32 prevSampleCount)
{
    ASSERT(scene);
    ASSERT(pCube);
    ASSERT(samples >= 0);
    ASSERT(prevSampleCount >= 0);

    pt_results_t results = pt_raygen(scene, origin, samples);

    const float4* pim_noalias colors = results.colors;
    const float4* pim_noalias directions = results.directions;

    AmbCube_t cube = *pCube;
    float w = 6.0f / (1.0f + samples);
    w = w / (1.0f + prevSampleCount);
    for (i32 i = 0; i < samples; ++i)
    {
        cube = AmbCube_Fit(cube, w, directions[i], colors[i]);
    }
    *pCube = cube;

    return prevSampleCount + 1;
}
