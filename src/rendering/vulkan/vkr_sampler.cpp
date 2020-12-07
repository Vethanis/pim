#include "rendering/vulkan/vkr_sampler.h"

#include "containers/dict.hpp"
#include <string.h>

struct SamplerInfo
{
    VkFilter filter;
    VkSamplerMipmapMode mipMode;
    VkSamplerAddressMode addressMode;
    float aniso;

    bool operator==(const SamplerInfo& other) const
    {
        return memcmp(this, &other, sizeof(*this)) == 0;
    }
};

static Dict<SamplerInfo, VkSampler> ms_samplerCache;

static VkSampler vkrSampler_New(
    VkFilter filter,
    VkSamplerMipmapMode mipMode,
    VkSamplerAddressMode addressMode,
    float aniso)
{
    ASSERT((aniso == 0.0f) || g_vkr.phdevFeats.samplerAnisotropy);
    VkSamplerCreateInfo info = {};
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

static void vkrSampler_Del(VkSampler sampler)
{
    if (sampler)
    {
        vkDestroySampler(g_vkr.dev, sampler, NULL);
    }
}

PIM_C_BEGIN

bool vkrSampler_Init(void)
{
    return true;
}

void vkrSampler_Update(void)
{

}

void vkrSampler_Shutdown(void)
{
    for (auto pair : ms_samplerCache)
    {
        vkrSampler_Del(pair.value);
    }
    ms_samplerCache.Reset();
}

VkSampler vkrSampler_Get(
    VkFilter filter,
    VkSamplerMipmapMode mipMode,
    VkSamplerAddressMode addressMode,
    float aniso)
{
    SamplerInfo key = {};
    key.filter = filter;
    key.mipMode = mipMode;
    key.addressMode = addressMode;
    key.aniso = aniso;

    VkSampler* pSampler = ms_samplerCache.Get(key);
    if (pSampler)
    {
        return *pSampler;
    }
    else
    {
        VkSampler sampler = vkrSampler_New(filter, mipMode, addressMode, aniso);
        if (sampler)
        {
            ms_samplerCache.AddCopy(key, sampler);
        }
        return sampler;
    }
}

PIM_C_END
