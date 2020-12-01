#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

VkDescriptorSetLayout vkrDescSetLayout_New(
    i32 bindingCount,
    const VkDescriptorSetLayoutBinding* pBindings,
    VkDescriptorSetLayoutCreateFlags flags);
void vkrDescSetLayout_Del(VkDescriptorSetLayout layout);

VkDescriptorPool vkrDescPool_New(
    i32 maxSets,
    i32 sizeCount,
    const VkDescriptorPoolSize* sizes);
void vkrDescPool_Del(VkDescriptorPool pool);
void vkrDescPool_Reset(VkDescriptorPool pool);

VkDescriptorSet vkrDescSet_New(VkDescriptorPool pool, VkDescriptorSetLayout layout);
void vkrDescSet_Del(VkDescriptorPool pool, VkDescriptorSet set);

void vkrDesc_WriteImageTable(
    VkDescriptorSet set, i32 binding,
    VkDescriptorType type,
    i32 count, const VkDescriptorImageInfo* bindings);

void vkrDesc_WriteBufferTable(
    VkDescriptorSet set, i32 binding,
    VkDescriptorType type,
    i32 count, const VkDescriptorBufferInfo* bindings);

PIM_C_END
