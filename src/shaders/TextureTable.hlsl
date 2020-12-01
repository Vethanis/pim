#ifndef TEXTURE_TABLE_HLSL
#define TEXTURE_TABLE_HLSL

#include "bindings.hlsl"

float4 SampleTable(uint index, float2 uv)
{
    return TextureTable2D[index].Sample(SamplerTable2D[index], uv);
}

#endif // TEXTURE_TABLE_HLSL
