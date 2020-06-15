#include "math/ambcube.h"
#include "rendering/path_tracer.h"
#include "math/float3_funcs.h"

i32 AmbCube_Bake(
    const struct pt_scene_s* scene,
    AmbCube_t* pCube,
    float4 origin,
    i32 samples,
    i32 prevSampleCount,
    i32 bounces)
{
    ASSERT(scene);
    ASSERT(pCube);
    ASSERT(samples >= 0);
    ASSERT(prevSampleCount >= 0);
    ASSERT(bounces >= 0);

    ray_t ray = { .ro = origin };

    pt_results_t results = pt_raygen(scene, ray, ptdist_sphere, samples, bounces);

    const float4* pim_noalias colors = results.colors;
    const float4* pim_noalias directions = results.directions;

    AmbCube_t cube = *pCube;
    i32 s = prevSampleCount;
    float w = 6.0f / (1.0f + samples);
    w = w / (1.0f + s);
    for (i32 i = 0; i < samples; ++i)
    {
        cube = AmbCube_Fit(cube, w, directions[i], colors[i]);
    }
    *pCube = cube;

    return prevSampleCount + 1;
}
