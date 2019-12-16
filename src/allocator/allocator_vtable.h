#pragma once

#include "common/int_types.h"

namespace Allocator
{
    struct Header
    {
        i32 type;
        i32 size;
        i32 c;
        i32 d;
    };

    union UAllocator;

    struct VTable
    {
        using InitFn = void(*)(UAllocator& inst, void* memory, i32 bytes);
        using ShutdownFn = void(*)(UAllocator& inst);
        using ClearFn = void(*)(UAllocator& inst);
        using AllocFn = Header * (*)(UAllocator& inst, i32 count);
        using ReallocFn = Header * (*)(UAllocator& inst, Header* prev, i32 count);
        using FreeFn = void(*)(UAllocator& inst, Header* prev);

        const InitFn Init;
        const ShutdownFn Shutdown;
        const ClearFn Clear;
        const AllocFn Alloc;
        const ReallocFn Realloc;
        const FreeFn Free;

        template<typename T>
        static T& As(UAllocator& inst)
        {
            return reinterpret_cast<T&>(inst);
        }

        template<typename T>
        static void InitAs(UAllocator& inst, void* memory, i32 bytes)
        {
            As<T>(inst).Init(memory, bytes);
        }

        template<typename T>
        static void ShutdownAs(UAllocator& inst)
        {
            As<T>(inst).Shutdown();
        }

        template<typename T>
        static void ClearAs(UAllocator& inst)
        {
            As<T>(inst).Clear();
        }

        template<typename T>
        static Header* AllocAs(UAllocator& inst, i32 count)
        {
            return As<T>(inst).Alloc(count);
        }

        template<typename T>
        static Header* ReallocAs(UAllocator& inst, Header* prev, i32 count)
        {
            return As<T>(inst).Realloc(prev, count);
        }

        template<typename T>
        static void FreeAs(UAllocator& inst, Header* prev)
        {
            As<T>(inst).Free(prev);
        }

        template<typename T>
        static constexpr const VTable Create()
        {
            return VTable
            {
                InitAs<T>,
                ShutdownAs<T>,
                ClearAs<T>,
                AllocAs<T>,
                ReallocAs<T>,
                FreeAs<T>,
            };
        }
    };
};
