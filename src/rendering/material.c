#include "rendering/material.h"
#include "math/color.h"
#include "rendering/sampler.h"

float4 VEC_CALL material_albedo(const material_t* mat, float2 uv)
{
    float4 value = ColorToLinear(mat->flatAlbedo);
    texture_t tex;
    if (texture_get(mat->albedo, &tex))
    {
        value = f4_mul(value, UvBilinearWrap_c32(tex.texels, tex.size, uv));
    }
    return f4_saturate(value);
}

float4 VEC_CALL material_rome(const material_t* mat, float2 uv)
{
    float4 value = ColorToLinear(mat->flatRome);
    texture_t tex;
    if (texture_get(mat->rome, &tex))
    {
        value = f4_mul(value, UvBilinearWrap_c32(tex.texels, tex.size, uv));
    }
    return f4_saturate(value);
}

float4 VEC_CALL material_normal(const material_t* mat, float2 uv)
{
    float4 value = { 0.0f, 0.0f, 1.0f, 0.0f };
    texture_t tex;
    if (texture_get(mat->normal, &tex))
    {
        value = UvBilinearWrap_dir8(tex.texels, tex.size, uv);
    }
    return value;
}
