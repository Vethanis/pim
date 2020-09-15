#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrTexTable_New(vkrTexTable* table);
void vkrTexTable_Del(vkrTexTable* table);

void vkrTexTable_Update(vkrTexTable* table);
void vkrTexTable_Write(const vkrTexTable* table, VkDescriptorSet set);

PIM_C_END
