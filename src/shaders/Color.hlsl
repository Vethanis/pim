#ifndef COLOR_HLSL
#define COLOR_HLSL

#define kEmissionScale 100.0f

float3 UnpackEmission(float3 albedo, float e)
{
    e = e * e * kEmissionScale;
    return albedo * e;
}

float3 Xy16ToNormalTs(float2 xy)
{
    float3 n;
    n.x = xy.x;
    n.y = xy.y;
    n.z = sqrt(max(0.0f, 1.0f - (n.x * n.x + n.y * n.y)));
    return n;
}

// http://filmicworlds.com/blog/filmic-tonemapping-operators/
float3 TonemapUncharted2(float3 x)
{
    const float a = 0.15;
    const float b = 0.50;
    const float c = 0.10;
    const float d = 0.20;
    const float e = 0.02;
    const float f = 0.30;
    return ((x * (a * x + c * b) + d * e) / (x * (a * x + b) + d * f)) - e / f;
}

// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
float3 TonemapACES(float3 x)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return (x * (a * x + b)) / (x * (c * x + d) + e);
}

float PerceptualLuminance(float3 color)
{
    return dot(color, float3(0.2126, 0.7152, 0.0722));
}

#endif // COLOR_HLSL
