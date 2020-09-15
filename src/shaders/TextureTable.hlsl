#ifndef TEXTURE_TABLE_HLSL
#define TEXTURE_TABLE_HLSL

[[vk::binding(2)]]
SamplerState kSamplers[];
[[vk::binding(2)]]
Texture2D kTextures[];

float4 SampleTable(uint index, float2 uv)
{
    return kTextures[index].Sample(kSamplers[index], uv);
}

#endif // TEXTURE_TABLE_HLSL
