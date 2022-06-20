#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrBuffer_New(
    vkrBuffer *const buffer,
    i32 size,
    VkBufferUsageFlags usage,
    vkrMemUsage memUsage);
void vkrBuffer_Release(vkrBuffer *const buffer);

// if size exceeds buffer's current size:
// - creates new larger buffer
// - releases old buffer with fence
// - updates buffer inout argument
// - contents of buffer are invalidated by this call!
bool vkrBuffer_Reserve(
    vkrBuffer *const buffer,
    i32 size,
    VkBufferUsageFlags bufferUsage,
    vkrMemUsage memUsage);

const void* vkrBuffer_MapRead(vkrBuffer* buffer);
void vkrBuffer_UnmapRead(vkrBuffer* buffer);
void* vkrBuffer_MapWrite(vkrBuffer* buffer);
void vkrBuffer_UnmapWrite(vkrBuffer* buffer);

// helper for map, unmap, flush
bool vkrBuffer_Write(
    vkrBuffer* buffer,
    const void* src,
    i32 size);
bool vkrBuffer_Read(
    vkrBuffer* buffer,
    void* dst,
    i32 size);

// ----------------------------------------------------------------------------

bool vkrBufferSet_New(
    vkrBufferSet *const set,
    i32 size,
    VkBufferUsageFlags usage,
    vkrMemUsage memUsage);
void vkrBufferSet_Release(vkrBufferSet *const set);
vkrBuffer *const vkrBufferSet_Current(vkrBufferSet *const set);
vkrBuffer *const vkrBufferSet_Next(vkrBufferSet *const set);
vkrBuffer *const vkrBufferSet_Prev(vkrBufferSet *const set);
bool vkrBufferSet_Reserve(
    vkrBufferSet *const set,
    i32 size,
    VkBufferUsageFlags bufferUsage,
    vkrMemUsage memUsage);
bool vkrBufferSet_Write(
    vkrBufferSet *const set,
    const void* src,
    i32 size);
bool vkrBufferSet_Read(
    vkrBufferSet *const set,
    void* dst,
    i32 size);

PIM_C_END
