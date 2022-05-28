#include "rendering/vulkan/vkr_extension.h"
#include "rendering/vulkan/vkr_device.h"
#include "rendering/vulkan/vkr_instance.h"
#include "allocator/allocator.h"
#include "common/console.h"
#include "common/stringutil.h"
#include "math/scalar.h"
#include <string.h>
#include <GLFW/glfw3.h>

i32 vkrFindExtension(
    const VkExtensionProperties* props,
    u32 count,
    const char* name)
{
    ASSERT(props || !count);
    ASSERT(name);
    for (u32 i = 0; i < count; ++i)
    {
        if (StrCmp(ARGS(props[i].extensionName), name) == 0)
        {
            return (i32)i;
        }
    }
    return -1;
}

i32 vkrFindLayer(
    const VkLayerProperties* props,
    u32 count,
    const char* name)
{
    ASSERT(props || !count);
    ASSERT(name);
    for (u32 i = 0; i < count; ++i)
    {
        if (StrCmp(ARGS(props[i].layerName), name) == 0)
        {
            return (i32)i;
        }
    }
    return -1;
}

bool vkrTryAddExtension(
    StrList* list,
    const VkExtensionProperties* props,
    u32 propCount,
    const char* name)
{
    if (vkrFindExtension(props, propCount, name) >= 0)
    {
        StrList_Add(list, name);
        return true;
    }
    else
    {
        Con_Logf(LogSev_Warning, "vkr", "Failed to load extension '%s'", name);
        return false;
    }
}

bool vkrTryAddLayer(
    StrList* list,
    const VkLayerProperties* props,
    u32 propCount,
    const char* name)
{
    if (vkrFindLayer(props, propCount, name) >= 0)
    {
        StrList_Add(list, name);
        return true;
    }
    else
    {
        Con_Logf(LogSev_Warning, "vkr", "Failed to load layer '%s'", name);
        return false;
    }
}

void vkrDevExts_New(vkrDevExts* exts, VkPhysicalDevice phdev)
{
    ASSERT(phdev);
    u32 ct = 0;
    const VkExtensionProperties* props = vkrEnumDevExtensions(phdev, &ct);
    memset(exts, 0, sizeof(*exts));
#define ADD_EXT(name) exts->name = vkrFindExtension(props, ct, CAT_TOK("VK_", STR_TOK(name))) >= 0;
    VKR_DEV_EXTS(ADD_EXT);
#undef ADD_EXT
}

StrList vkrDevExts_ToList(const vkrDevExts* exts)
{
    StrList list;
    StrList_New(&list, EAlloc_Temp);
#define ADD_EXT(name) if(exts->name) { StrList_Add(&list, CAT_TOK("VK_", STR_TOK(name))); }
    VKR_DEV_EXTS(ADD_EXT);
#undef ADD_EXT
    return list;
}

StrList vkrGetLayers(vkrLayers* layersOut)
{
    memset(layersOut, 0, sizeof(*layersOut));
    StrList list;
    StrList_New(&list, EAlloc_Temp);
    u32 ct = 0;
    const VkLayerProperties* props = vkrEnumInstLayers(&ct);
#define ADD_EXT(name) layersOut->name = vkrTryAddLayer(&list, props, ct, CAT_TOK("VK_LAYER_", STR_TOK(name)));
    VKR_LAYERS(ADD_EXT);
#undef ADD_EXT
    return list;
}

StrList vkrGetInstExtensions(vkrInstExts* extsOut)
{
    memset(extsOut, 0, sizeof(*extsOut));
    StrList list;
    StrList_New(&list, EAlloc_Temp);
    u32 ct = 0;
    const VkExtensionProperties* props = vkrEnumInstExtensions(&ct);
    u32 glfwCount = 0;
    const char** glfwList = glfwGetRequiredInstanceExtensions(&glfwCount);
    for (u32 i = 0; i < glfwCount; ++i)
    {
        vkrTryAddExtension(&list, props, ct, glfwList[i]);
    }
#define ADD_EXT(name) extsOut->name = vkrTryAddExtension(&list, props, ct, CAT_TOK("VK_", STR_TOK(name)));
    VKR_INST_EXTS(ADD_EXT);
#undef ADD_EXT
    return list;
}

StrList vkrGetDevExtensions(VkPhysicalDevice phdev, vkrDevExts* extsOut)
{
    vkrDevExts_New(extsOut, phdev);
    return vkrDevExts_ToList(extsOut);
}

VkLayerProperties* vkrEnumInstLayers(u32* countOut)
{
    ASSERT(countOut);
    u32 count = 0;
    VkLayerProperties* props = NULL;
    VkCheck(vkEnumerateInstanceLayerProperties(&count, NULL));
    Temp_Reserve(props, count);
    VkCheck(vkEnumerateInstanceLayerProperties(&count, props));
    *countOut = count;
    return props;
}

VkExtensionProperties* vkrEnumInstExtensions(u32* countOut)
{
    ASSERT(countOut);
    u32 count = 0;
    VkExtensionProperties* props = NULL;
    VkCheck(vkEnumerateInstanceExtensionProperties(NULL, &count, NULL));
    Temp_Reserve(props, count);
    VkCheck(vkEnumerateInstanceExtensionProperties(NULL, &count, props));
    *countOut = count;
    return props;
}

VkExtensionProperties* vkrEnumDevExtensions(
    VkPhysicalDevice phdev,
    u32* countOut)
{
    ASSERT(phdev);
    ASSERT(countOut);
    u32 count = 0;
    VkExtensionProperties* props = NULL;
    VkCheck(vkEnumerateDeviceExtensionProperties(phdev, NULL, &count, NULL));
    Temp_Reserve(props, count);
    VkCheck(vkEnumerateDeviceExtensionProperties(phdev, NULL, &count, props));
    *countOut = count;
    return props;
}

void vkrListInstLayers(void)
{
    u32 count = 0;
    VkLayerProperties* props = vkrEnumInstLayers(&count);
    Con_Logf(LogSev_Info, "vkr", "%d available instance layers", count);
    for (u32 i = 0; i < count; ++i)
    {
        Con_Logf(LogSev_Info, "vkr", props[i].layerName);
    }
}

void vkrListInstExtensions(void)
{
    u32 count = 0;
    VkExtensionProperties* props = vkrEnumInstExtensions(&count);
    Con_Logf(LogSev_Info, "vkr", "%d available instance extensions", count);
    for (u32 i = 0; i < count; ++i)
    {
        Con_Logf(LogSev_Info, "vkr", props[i].extensionName);
    }
}

void vkrListDevExtensions(VkPhysicalDevice phdev)
{
    ASSERT(phdev);
    u32 count = 0;
    VkExtensionProperties* props = vkrEnumDevExtensions(phdev, &count);
    Con_Logf(LogSev_Info, "vkr", "%d available device extensions", count);
    for (u32 i = 0; i < count; ++i)
    {
        Con_Logf(LogSev_Info, "vkr", props[i].extensionName);
    }
}

i32 vkrFeats_Eval(const vkrFeats* feats)
{
    i32 score = 0;

    // ------------------------------------------------------------------------
    // https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkPhysicalDeviceAccelerationStructureFeaturesKHR.html

    // https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#acceleration-structure
    score += feats->accstr.accelerationStructure ? 64 : 0;

    // https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#vkCmdBuildAccelerationStructuresIndirectKHR
    //score += feats->accstr.accelerationStructureIndirectBuild ? 16 : 0;

    // https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#features-accelerationStructureHostCommands
    //score += feats->accstr.accelerationStructureHostCommands ? 16 : 0;

    // https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#features-descriptorBindingAccelerationStructureUpdateAfterBind
    //score += feats->accstr.descriptorBindingAccelerationStructureUpdateAfterBind ? 4 : 0;

    // ------------------------------------------------------------------------
    // https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkPhysicalDeviceRayTracingPipelineFeaturesKHR.html

    // https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#ray-tracing
    score += feats->rtpipe.rayTracingPipeline ? 64 : 0;

    // https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#vkCmdTraceRaysIndirectKHR
    //score += feats->rtpipe.rayTracingPipelineTraceRaysIndirect ? 16 : 0;

    // https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#ray-traversal-culling-primitive
    //score += feats->rtpipe.rayTraversalPrimitiveCulling ? 16 : 0;

    // ------------------------------------------------------------------------
    // https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkPhysicalDeviceRayQueryFeaturesKHR.html

    // https://github.com/KhronosGroup/SPIRV-Registry/blob/master/extensions/KHR/SPV_KHR_ray_query.asciidoc
    score += feats->rquery.rayQuery ? 64 : 0;

    // ------------------------------------------------------------------------
    // https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkPhysicalDeviceFeatures.html

    // highly useful things
    score += feats->phdev.features.fullDrawIndexUint32 ? 16 : 0;
    score += feats->phdev.features.samplerAnisotropy ? 16 : 0;
    score += feats->phdev.features.textureCompressionBC ? 16 : 0;
    score += feats->phdev.features.independentBlend ? 16 : 0;

    // debug drawing
    score += feats->phdev.features.fillModeNonSolid ? 2 : 0;
    score += feats->phdev.features.wideLines ? 2 : 0;
    score += feats->phdev.features.largePoints ? 2 : 0;

    // profiling
    // https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkQueryPipelineStatisticFlagBits.html
    score += feats->phdev.features.pipelineStatisticsQuery ? 2 : 0;

    // shader features
    score += feats->phdev.features.fragmentStoresAndAtomics ? 4 : 0;
    score += feats->phdev.features.shaderInt64 ? 4 : 0;
    score += feats->phdev.features.shaderInt16 ? 1 : 0;
    score += feats->phdev.features.shaderStorageImageExtendedFormats ? 4 : 0;

    // dynamic indexing
    score += feats->phdev.features.shaderUniformBufferArrayDynamicIndexing ? 4 : 0;
    score += feats->phdev.features.shaderStorageBufferArrayDynamicIndexing ? 4 : 0;
    score += feats->phdev.features.shaderSampledImageArrayDynamicIndexing ? 4 : 0;
    score += feats->phdev.features.shaderStorageImageArrayDynamicIndexing ? 4 : 0;
    score += feats->phdev.features.imageCubeArray ? 1 : 0;

    // indirect and conditional rendering
    score += feats->phdev.features.fullDrawIndexUint32 ? 1 : 0;
    score += feats->phdev.features.multiDrawIndirect ? 1 : 0;
    score += feats->phdev.features.drawIndirectFirstInstance ? 1 : 0;

    return score;
}

i32 vkrProps_Eval(const vkrProps* props)
{
    i32 score = 0;
    score += vkrProps_LimitsEval(&props->phdev.properties.limits);
    score += vkrProps_AccStrEval(&props->accstr);
    score += vkrProps_RtPipeEval(&props->rtpipe);
    return score;
}

i32 vkrProps_LimitsEval(const VkPhysicalDeviceLimits* lims)
{
    i32 score = 0;
    score += u64_log2(lims->maxImageDimension1D);
    score += u64_log2(lims->maxImageDimension2D);
    score += u64_log2(lims->maxImageDimension3D);
    score += u64_log2(lims->maxImageDimensionCube);
    score += u64_log2(lims->maxImageArrayLayers);
    score += u64_log2(lims->maxUniformBufferRange);
    score += u64_log2(lims->maxStorageBufferRange);
    score += u64_log2(lims->maxPushConstantsSize);
    score += u64_log2(lims->maxMemoryAllocationCount);
    score += u64_log2(lims->maxPerStageDescriptorStorageBuffers);
    score += u64_log2(lims->maxPerStageDescriptorSampledImages);
    score += u64_log2(lims->maxPerStageDescriptorStorageImages);
    score += u64_log2(lims->maxPerStageResources);
    score += u64_log2(lims->maxVertexInputAttributes);
    score += u64_log2(lims->maxVertexInputBindings);
    score += u64_log2(lims->maxFragmentInputComponents);
    score += u64_log2(lims->maxComputeSharedMemorySize);
    score += u64_log2(lims->maxComputeWorkGroupInvocations);
    score += u64_log2(lims->maxDrawIndirectCount);
    score += u64_log2(lims->maxFramebufferWidth);
    score += u64_log2(lims->maxFramebufferHeight);
    score += u64_log2(lims->maxColorAttachments);
    return score;
}

static i32 vkrProps_AccStrEval(const VkPhysicalDeviceAccelerationStructurePropertiesKHR* accstr)
{
    i32 score = 0;
    score += u64_log2(accstr->maxGeometryCount);
    score += u64_log2(accstr->maxInstanceCount);
    score += u64_log2(accstr->maxPrimitiveCount);
    score += u64_log2(accstr->maxPerStageDescriptorAccelerationStructures);
    score += u64_log2(accstr->maxDescriptorSetAccelerationStructures);
    score += u64_log2(accstr->maxDescriptorSetUpdateAfterBindAccelerationStructures);
    return score;
}

static i32 vkrProps_RtPipeEval(const VkPhysicalDeviceRayTracingPipelinePropertiesKHR* rtpipe)
{
    i32 score = 0;
    score += u64_log2(rtpipe->maxRayRecursionDepth);
    score += u64_log2(rtpipe->maxRayDispatchInvocationCount);
    score += u64_log2(rtpipe->maxRayHitAttributeSize);
    return score;
}

i32 vkrDevExts_OptEval(const vkrDevExts* exts)
{
    i32 score = 0;
    score += exts->EXT_memory_budget ? 1 : 0;
    score += exts->EXT_hdr_metadata ? 1 : 0;
    score += exts->KHR_shader_float16_int8 ? 1 : 0;
    score += exts->KHR_16bit_storage ? 1 : 0;
    score += exts->KHR_push_descriptor ? 1 : 0;
    score += exts->EXT_memory_priority ? 1 : 0;
    score += exts->KHR_bind_memory2 ? 1 : 0;
    score += exts->KHR_shader_float_controls ? 1 : 0;
    score += exts->KHR_spirv_1_4 ? 1 : 0;
    score += exts->EXT_conditional_rendering ? 1 : 0;
    score += exts->KHR_draw_indirect_count ? 1 : 0;
    return score;
}

i32 vkrDevExts_RtEval(const vkrDevExts* exts)
{
    i32 score = 0;
    score += (exts->KHR_acceleration_structure && exts->KHR_ray_tracing_pipeline) ? 1 : 0;
    score += exts->KHR_ray_query ? 1 : 0;
    return score;
}

bool vkrDevExts_ReqEval(const vkrDevExts* exts)
{
    bool hasAll = true;
    hasAll &= exts->KHR_swapchain;
    return hasAll;
}
