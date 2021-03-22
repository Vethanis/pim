#include "rendering/vulkan/vkr_sampler.h"
#include "containers/dict.h"
#include <string.h>

typedef struct SamplerInfo_s
{
    u8 filter : 1; // VkFilter
    u8 mipMode : 1; // VkSamplerMipmapMode
    u8 addressMode : 2; // VkSamplerAddressMode
    u8 aniso : 5; // float [0, 16]
} SamplerInfo;

static VkSampler vkrSampler_New(
    VkFilter filter,
    VkSamplerMipmapMode mipMode,
    VkSamplerAddressMode addressMode,
    float aniso);

// ----------------------------------------------------------------------------

// SamplerInfo -> VkSampler
static Dict ms_samplerCache;

bool vkrSampler_Init(void)
{
    Dict_New(&ms_samplerCache, sizeof(SamplerInfo), sizeof(VkSampler), EAlloc_Perm);
    return true;
}

void vkrSampler_Update(void)
{

}

void vkrSampler_Shutdown(void)
{
    const i32 len = ms_samplerCache.width;
    VkSampler* samplers = ms_samplerCache.values;
    for (i32 i = 0; i < len; ++i)
    {
        if (samplers[i])
        {
            vkDestroySampler(g_vkr.dev, samplers[i], NULL);
            samplers[i] = NULL;
        }
    }
    Dict_Del(&ms_samplerCache);
}

VkSampler vkrSampler_Get(
    VkFilter filter,
    VkSamplerMipmapMode mipMode,
    VkSamplerAddressMode addressMode,
    float aniso)
{
    const SamplerInfo key = {
        .filter = filter,
        .mipMode = mipMode,
        .addressMode = addressMode,
        .aniso = aniso,
    };
    VkSampler sampler = NULL;
    if (Dict_Get(&ms_samplerCache, &key, &sampler))
    {
        return sampler;
    }
    sampler = vkrSampler_New(filter, mipMode, addressMode, aniso);
    if (sampler)
    {
        Dict_Add(&ms_samplerCache, &key, &sampler);
    }
    return sampler;
}

// ----------------------------------------------------------------------------

static VkSampler vkrSampler_New(
    VkFilter filter,
    VkSamplerMipmapMode mipMode,
    VkSamplerAddressMode addressMode,
    float aniso)
{
    ASSERT((aniso == 0.0f) || g_vkr.phdevFeats.samplerAnisotropy);
    VkSamplerCreateInfo info = { 0 };
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter = filter;
    info.minFilter = filter;
    info.mipmapMode = mipMode;
    info.addressModeU = addressMode;
    info.addressModeV = addressMode;
    info.addressModeW = addressMode;
    info.mipLodBias = 0.0f;
    info.anisotropyEnable = aniso > 1.0f;
    info.maxAnisotropy = aniso;
    info.minLod = 0.0f;
    info.maxLod = VK_LOD_CLAMP_NONE;
    VkSampler handle = NULL;
    VkCheck(vkCreateSampler(g_vkr.dev, &info, NULL, &handle));
    ASSERT(handle);
    return handle;
}
