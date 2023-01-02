#ifndef R_CONFIG_H
#define R_CONFIG_H

// shaders must include common.hlsl before this file
#ifndef PIM_HLSL
#   define PIM_C                        1
#   define PIM_STRUCT_BEGIN(name)       typedef struct
#   define PIM_STRUCT_END(name)         name
#endif // PIM_HLSL

// ----------------------------------------------------------------------------

// number of swapchain images
#define R_MaxSwapchainLen        (3)
#define R_DesiredSwapchainLen    (2)
// number of resources for dynamic buffers and images
#define R_ResourceSets           (3)
#define R_CmdsPerQueue           (64)

#define R_AnisotropySamples      (4)

// ----------------------------------------------------------------------------
// resource bindings

// texture tables

#define bid_TextureTable1D          (0)
#define bid_TextureTable1D_TYPE     (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
#define bid_TextureTable1D_COUNT    (64)
#define bid_TextureTable1D_STAGES   (VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT)

#define bid_TextureTable2D          (bid_TextureTable1D + 1)
#define bid_TextureTable2D_TYPE     (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
#define bid_TextureTable2D_COUNT    (512)
#define bid_TextureTable2D_STAGES   (VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT)

#define bid_TextureTable3D          (bid_TextureTable2D + 1)
#define bid_TextureTable3D_TYPE     (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
#define bid_TextureTable3D_COUNT    (64)
#define bid_TextureTable3D_STAGES   (VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT)

#define bid_TextureTableCube        (bid_TextureTable3D + 1)
#define bid_TextureTableCube_TYPE   (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
#define bid_TextureTableCube_COUNT  (64)
#define bid_TextureTableCube_STAGES (VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT)

#define bid_TextureTable2DArray         (bid_TextureTableCube + 1)
#define bid_TextureTable2DArray_TYPE    (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
#define bid_TextureTable2DArray_COUNT   (64)
#define bid_TextureTable2DArray_STAGES  (VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT)

// loose bindings

#define bid_Globals                 (bid_TextureTable2DArray + 1)
#define bid_Globals_TYPE            (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
#define bid_Globals_STAGES          (VK_SHADER_STAGE_ALL)

#define bid_SceneLuminance          (bid_Globals + 1)
#define bid_SceneLuminance_TYPE     (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
#define bid_SceneLuminance_STAGES   (VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT)

#define bid_RWSceneLuminance        (bid_SceneLuminance + 1)
#define bid_RWSceneLuminance_TYPE   (VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
#define bid_RWSceneLuminance_STAGES (VK_SHADER_STAGE_COMPUTE_BIT)

#define bid_HistogramBuffer         (bid_RWSceneLuminance + 1)
#define bid_HistogramBuffer_TYPE    (VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
#define bid_HistogramBuffer_STAGES  (VK_SHADER_STAGE_COMPUTE_BIT)

#define bid_ExposureBuffer          (bid_HistogramBuffer + 1)
#define bid_ExposureBuffer_TYPE     (VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
#define bid_ExposureBuffer_STAGES   (VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT)

#define bid_COUNT                   (bid_ExposureBuffer + 1)

// descriptor pool sizes

// bid_Globals
#define DescPool_UniformBuffer_COUNT (1)

// bid_RWSceneLuminance
#define DescPool_StorageImage_COUNT (1)

// bid_HistogramBuffer
// bid_ExposureBuffer
#define DescPool_StorageBuffer_COUNT (2)

// bid_TextureTable1D
// bid_TextureTable2D
// bid_TextureTable3D
// bid_TextureTableCube
// bid_TextureTable2DArray
// bid_SceneLuminance
#define DescPool_CombinedImageSampler_COUNT (\
    bid_TextureTable1D_COUNT + \
    bid_TextureTable2D_COUNT + \
    bid_TextureTable3D_COUNT + \
    bid_TextureTableCube_COUNT + \
    bid_TextureTable2DArray_COUNT + \
    bid_SceneLuminance)

// ----------------------------------------------------------------------------

#define ENABLE_HDR 1

// colorspace
//#define COLOR_SCENE_REC709      1
//#define COLOR_SCENE_REC2020     1
#define COLOR_SCENE_AP1         1
//#define COLOR_SCENE_AP0         1

// packed emission range
#define kEmissionScale          100.0f

// ----------------------------------------------------------------------------

// exposure
#define R_ExposureHistogramSize  (256)

// ----------------------------------------------------------------------------

#endif // R_CONFIG_H
