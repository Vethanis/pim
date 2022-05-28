#ifndef RANDOM_HLSL
#define RANDOM_HLSL

// https://jcgt.org/published/0009/03/02/supplementary.pdf#page=49
uint Pcg1D(uint v)
{
    uint state = v * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ˆ state) * 277803737u;
    return (word >> 22u) ˆ word;
}

// https://jcgt.org/published/0009/03/02/supplementary.pdf#page=52
uint2 Pcg2D(uint2 v)
{
    v = v * 1664525u + 1013904223u;
    v.x += v.y * 1664525u;
    v.y += v.x * 1664525u;
    v = v ˆ(v >> 16u);
    v.x += v.y * 1664525u;
    v.y += v.x * 1664525u;
    v = v ˆ(v >> 16u);
    return v;
}

// https://jcgt.org/published/0009/03/02/supplementary.pdf#page=53
uint3 Pcg3D(uint3 v)
{
    v = v * 1664525u + 1013904223u;
    v.x += v.y*v.z;
    v.y += v.z*v.x;
    v.z += v.x*v.y;
    v = v ˆ(v >> 16u);
    v.x += v.y*v.z;
    v.y += v.z*v.x;
    v.z += v.x*v.y;
    return v;
}

// https://jcgt.org/published/0009/03/02/supplementary.pdf#page=55
uint4 Pcg4D(uint4 v)
{
    v = v * 1664525u + 1013904223u;
    v.x += v.y*v.w;
    v.y += v.z*v.x;
    v.z += v.x*v.y;
    v.w += v.y*v.z;
    v = v ˆ(v >> 16u);
    v.x += v.y*v.w;
    v.y += v.z*v.x;
    v.z += v.x*v.y;
    v.w += v.y*v.z;
    return v;
}

struct Rng
{
    uint4 state;
};
Rng Rng_FromThread(uint3 tid, uint seed)
{
    Rng rng;
    rng.state = uint4(tid.xyz, seed);
    return rng;
}
Rng Rng_FromCoord(uint2 xy, uint seed)
{
    return Rng_FromThread(uint3(xy, 0), seed);
}
Rng Rng_FromUv(float2 uv, uint2 size, uint seed)
{
    return Rng_FromCoord(uint2(uv * size + 0.5), seed);
}

uint4 Rng_SampleUint4(inout Rng rng)
{
    rng.state = Pcg4D(rng.state);
    return rng.state;
}
uint3 Rng_SampleUint3(inout Rng rng)
{
    rng.state = Pcg4D(rng.state);
    return rng.state.xyz;
}
uint2 Rng_SampleUint2(inout Rng rng)
{
    rng.state = Pcg4D(rng.state);
    return rng.state.xy;
}
uint Rng_SampleUint(inout Rng rng)
{
    rng.state = Pcg4D(rng.state);
    return rng.state.x;
}

float4 Rng_SampleFloat4(inout Rng rng)
{
    return (Rng_SampleUint4(rng) & 0xffffff) * (1.0 / 0xffffff);
}
float3 Rng_SampleFloat3(inout Rng rng)
{
    return (Rng_SampleUint3(rng) & 0xffffff) * (1.0 / 0xffffff);
}
float2 Rng_SampleFloat2(inout Rng rng)
{
    return (Rng_SampleUint2(rng) & 0xffffff) * (1.0 / 0xffffff);
}
float Rng_SampleFloat(inout Rng rng)
{
    return (Rng_SampleUint(rng) & 0xffffff) * (1.0 / 0xffffff);
}


#endif // RANDOM_HLSL
