#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

VkDescriptorPool vkrDescPool_New(
    i32 maxSets,
    i32 sizeCount,
    const VkDescriptorPoolSize* sizes);
void vkrDescPool_Del(VkDescriptorPool pool);
void vkrDescPool_Reset(VkDescriptorPool pool);

VkDescriptorSet vkrDesc_New(VkDescriptorPool pool, VkDescriptorSetLayout layout);
void vkrDesc_Del(VkDescriptorPool pool, VkDescriptorSet set);

void vkrDesc_WriteBindings(
    i32 count,
    const vkrBinding* bindings);

void vkrDesc_WriteImageTable(
    VkDescriptorSet set,
    i32 binding,
    VkDescriptorType type,
    i32 count,
    const VkDescriptorImageInfo* bindings);

void vkrDesc_WriteBufferTable(
    VkDescriptorSet set,
    i32 binding,
    VkDescriptorType type,
    i32 count,
    const VkDescriptorBufferInfo* bindings);

PIM_C_END
