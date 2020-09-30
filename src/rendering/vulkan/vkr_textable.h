#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrTexTable_New(vkrTexTable* table);
void vkrTexTable_Del(vkrTexTable* table);

void vkrTexTable_Update(vkrTexTable* table);
void vkrTexTable_Write(const vkrTexTable* table, VkDescriptorSet set);

i32 vkrTexTable_AllocSlot(
	vkrTexTable* table,
	VkSampler sampler,
	VkImageView view,
	VkImageLayout layout);
void vkrTexTable_FreeSlot(vkrTexTable* table, i32 slot);

i32 vkrTexTable_AllocContiguous(vkrTexTable* table, i32 count);

void vkrTexTable_WriteSlot(
	vkrTexTable* table,
	i32 slot,
	VkSampler sampler,
	VkImageView view,
	VkImageLayout layout);

void vkrTexTable_ClearSlot(vkrTexTable* table, i32 slot);

PIM_C_END
