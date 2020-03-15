#include "allocator/allocator.h"

#include "common/module.h"
#include "../allocator_module/allocator_module.h"
#include <string.h>

static allocator_module_t ms_module;

namespace Allocator
{
    static constexpr i32 kKilobyte = 1 << 10;
    static constexpr i32 kMegabyte = 1 << 20;
    static constexpr i32 kGigabyte = 1 << 30;

    static const int32_t kInitCapacity = 0;
    static constexpr i32 kPermCapacity = 128 * kMegabyte;
    static constexpr i32 kTempCapacity = 4 * kMegabyte;
    static constexpr i32 kTaskCapacity = 1 * kMegabyte;

    static const int32_t kSizes[] =
    {
        kInitCapacity,
        kPermCapacity,
        kTempCapacity,
        kTaskCapacity,
    };

    void Init()
    {
        if (!pimod_register("allocator_module", &ms_module, sizeof(ms_module)))
        {
            ASSERT(0);
        }

        ms_module.Init(kSizes, NELEM(kSizes));
    }

    void Update()
    {
        ms_module.Update();
    }

    void Shutdown()
    {
        ms_module.Shutdown();
        memset(&ms_module, 0, sizeof(ms_module));
        pimod_unload("allocator_module");
    }

    // ------------------------------------------------------------------------

    void* Alloc(AllocType type, i32 bytes)
    {
        void* ptr = ms_module.Alloc((EAllocator)type, bytes);
        ASSERT(ptr);
        ASSERT(((intptr_t)ptr & 15) == 0);
        return ptr;
    }

    void* Realloc(AllocType type, void* pOld, i32 bytes)
    {
        void* pNew = Alloc(type, bytes);
        if (pOld && pNew)
        {
            int32_t* pHeader = (int32_t*)pOld - 4;
            int32_t oldBytes = pHeader[0];
            memcpy(pNew, pOld, oldBytes < bytes ? oldBytes : bytes);
        }
        Free(pOld);
        return pNew;
    }

    void Free(void* ptr)
    {
        ms_module.Free(ptr);
    }

    void* Calloc(AllocType type, i32 bytes)
    {
        void* ptr = Alloc(type, bytes);
        ASSERT(ptr);
        memset(ptr, 0, bytes);
        return ptr;
    }
};
