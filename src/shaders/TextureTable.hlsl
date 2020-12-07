#ifndef TEXTURE_TABLE_HLSL
#define TEXTURE_TABLE_HLSL

#include "bindings.hlsl"

float4 SampleTable1D(uint index, float u)
{
    return TextureTable1D[index].Sample(SamplerTable1D[index], u);
}

float4 SampleTable2D(uint index, float2 uv)
{
    return TextureTable2D[index].Sample(SamplerTable2D[index], uv);
}

float4 SampleTable3D(uint index, float3 uvw)
{
    return TextureTable3D[index].Sample(SamplerTable3D[index], uvw);
}

float4 SampleTableCube(uint index, float3 dir)
{
    return TextureTableCube[index].Sample(SamplerTableCube[index], dir);
}

float4 SampleTable2DArray(uint index, float2 uv, int layer)
{
    return TextureTable2DArray[index].Sample(SamplerTable2DArray[index], float3(uv.x, uv.y, layer));
}

#endif // TEXTURE_TABLE_HLSL
