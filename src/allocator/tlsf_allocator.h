#pragma once

#include "allocator/iallocator.h"
#include "os/thread.h"

struct TlsfAllocator final : IAllocator
{
public:
    TlsfAllocator(i32 bytes);
    ~TlsfAllocator();

    void Clear() final;
    void* Alloc(i32 bytes) final;
    void Free(void* ptr) final;
    void* Realloc(void* prev, i32 bytes) final;

private:
    OS::Mutex m_lock;
    void* m_impl;
    void* m_memory;
    i32 m_bytes;
};
