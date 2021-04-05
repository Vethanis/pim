#pragma once

#include "rendering/vulkan/vkr.h"
#include "containers/strlist.h"

PIM_C_BEGIN

typedef struct vkrPhDevScore_s
{
    i32 rtScore;
    i32 extScore;
    i32 featScore;
    i32 propScore;
    bool hasRequiredExts;
    bool hasQueueSupport;
} vkrPhDevScore;

i32 vkrFindExtension(
    const VkExtensionProperties* props,
    u32 count,
    const char* name);
i32 vkrFindLayer(
    const VkLayerProperties* props,
    u32 count,
    const char* name);

bool vkrTryAddExtension(
    StrList* list,
    const VkExtensionProperties* props,
    u32 propCount,
    const char* name);
bool vkrTryAddLayer(
    StrList* list,
    const VkLayerProperties* props,
    u32 propCount,
    const char* name);

VkLayerProperties* vkrEnumInstLayers(u32* countOut);
VkExtensionProperties* vkrEnumInstExtensions(u32* countOut);
VkExtensionProperties* vkrEnumDevExtensions(VkPhysicalDevice phdev, u32* countOut);

StrList vkrGetLayers(vkrLayers* layersOut);
StrList vkrGetInstExtensions(vkrInstExts* extsOut);
StrList vkrGetDevExtensions(VkPhysicalDevice phdev, vkrDevExts* extsOut);

void vkrListInstLayers(void);
void vkrListInstExtensions(void);
void vkrListDevExtensions(VkPhysicalDevice phdev);

void vkrDevExts_New(vkrDevExts* exts, VkPhysicalDevice phdev);
StrList vkrDevExts_ToList(const vkrDevExts* exts);

i32 vkrFeats_Eval(const vkrFeats* feats);

i32 vkrProps_Eval(const vkrProps* props);
i32 vkrProps_LimitsEval(const VkPhysicalDeviceLimits* lims);
i32 vkrProps_AccStrEval(const VkPhysicalDeviceAccelerationStructurePropertiesKHR* accstr);
i32 vkrProps_RtPipeEval(const VkPhysicalDeviceRayTracingPipelinePropertiesKHR* rtpipe);

i32 vkrDevExts_OptEval(const vkrDevExts* exts);
i32 vkrDevExts_RtEval(const vkrDevExts* exts);
bool vkrDevExts_ReqEval(const vkrDevExts* exts);

PIM_C_END
