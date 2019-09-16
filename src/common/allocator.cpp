#include "common/allocator.h"
#include "common/macro.h"
#include "containers/array.h"
#include <malloc.h>
#include <string.h>

namespace Allocator
{
    constexpr usize DefaultAlignment = 64;
    constexpr u32 NumPools = 16;
    constexpr u32 PoolLen = 256;

    struct Pool
    {
        FixedArray<usize, PoolLen> sizes;
        FixedArray<u8*, PoolLen> ptrs;

        inline bool Give(Allocation& alloc)
        {
            DebugAssert(!sizes.full());
            if (!sizes.full())
            {
                ptrs.grow() = alloc.begin();
                sizes.grow() = alloc.size();
                alloc = { 0, 0 };
                return true;
            }
            return false;
        }

        inline bool Take(usize want, Allocation& alloc)
        {
            const usize* szs = sizes.begin();
            const usize ct = sizes.size();
            for (usize i = 0; i < ct; ++i)
            {
                if (szs[i] >= want)
                {
                    alloc = { ptrs[i], szs[i] };
                    sizes.remove(i);
                    ptrs.remove(i);
                    return true;
                }
            }
            return false;
        }
    };

    static Pool ms_pools[NumPools];

    static u32 GetPoolIdx(usize size)
    {
        u32 i = 0;
        usize a = 64;
        while (size > a)
        {
            a *= 4;
            ++i;
        }
        return Min(i, NumPools);
    }

    static bool Give(Allocation& alloc)
    {
        return ms_pools[GetPoolIdx(alloc.size())].Give(alloc);
    }

    static bool Take(usize want, Allocation& alloc)
    {
        return ms_pools[GetPoolIdx(want)].Take(want, alloc);
    }

    Allocation _Alloc(usize want)
    {
        Allocation got = { 0, 0 };
        if (want)
        {
            want = AlignGrow(want, DefaultAlignment);
            if (!Take(want, got))
            {
                void* ptr = _aligned_malloc(want, DefaultAlignment);
                got = { (u8*)ptr, want };
            }
            DebugAssert(got.begin());
            memset(got.begin(), 0, got.size());
        }
        return got;
    }

    Allocation _Realloc(Allocation& prev, usize want)
    {
        Allocation got = { 0, 0 };
        if (!want)
        {
            _Free(prev);
        }
        else if (want > prev.size())
        {
            got = _Alloc(want);
            memcpy(got.begin(), prev.begin(), prev.size());
            _Free(prev);
        }
        else
        {
            got = prev;
        }
        return got;
    }

    void _Free(Allocation& prev)
    {
        if (prev.begin())
        {
            if (!Give(prev))
            {
                _aligned_free(prev.begin());
            }
        }
        prev = { 0, 0 };
    }
};
