#ifndef SAMPLING_HLSL
#define SAMPLING_HLSL

#include "common.hlsl"

float3x3 NormalToTBN(float3 N)
{
    const float3 kX = { 1.0, 0.0, 0.0 };
    const float3 kZ = { 0.0, 0.0, 1.0 };

    float3 a = abs(N.z) < 0.9 ? kZ : kX;
    float3 T = normalize(cross(a, N));
    float3 B = cross(N, T);

    return float3x3(T, B, N);
}

float3 TbnToWorld(float3x3 TBN, float3 nTS)
{
    float3 r = TBN[0] * nTS.x;
    float3 u = TBN[1] * nTS.y;
    float3 f = TBN[2] * nTS.z;
    float3 dir = r + u + f;
    return dir;
}

float3 TanToWorld(float3 normalWS, float3 normalTS)
{
    float3x3 TBN = NormalToTBN(normalWS);
    return TbnToWorld(TBN, normalTS);
}

#endif // SAMPLING_HLSL
