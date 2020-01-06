#pragma once

#include "common/chunk_allocator.h"
#include "os/thread.h"

template<typename T>
struct Pool
{
    static ChunkAllocator ms_chunks;
    static OS::SpinLock ms_lock;
    static bool ms_init;

    static T* Alloc()
    {
        ms_lock.Lock();
        if (!ms_init)
        {
            ms_chunks.Init(Alloc_Pool, sizeof(T));
            ms_init = true;
        }
        void* ptr = ms_chunks.Allocate();
        ms_lock.Unlock();
        return (T*)ptr;
    }

    static void Free(T* ptr)
    {
        if (ptr)
        {
            ms_lock.Lock();
            ms_chunks.Free(ptr);
            ms_lock.Unlock();
        }
    }
};

template<typename T>
ChunkAllocator Pool<T>::ms_chunks;

template<typename T>
OS::SpinLock Pool<T>::ms_lock;

template<typename T>
bool Pool<T>::ms_init;
