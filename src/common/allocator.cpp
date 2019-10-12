#include "common/allocator.h"
#include "common/macro.h"
#include "common/mem.h"
#include "containers/array.h"
#include <malloc.h>

namespace Allocator
{
    constexpr i32 NumPools = 32;
    constexpr i32 PoolLen = 64;

    struct Pool
    {
        FixedArray<i32, PoolLen> sizes;
        FixedArray<u8*, PoolLen> ptrs;
    };

    static Pool ms_pools[NumPools];

    static i32 GetPoolIdx(i32 size)
    {
        i32 i = 0;
        i32 a = 8;
        while (size > a)
        {
            a <<= 1;
            ++i;
        }
        return Min(i, NumPools - 1);
    }

    static bool Give(Allocation& alloc)
    {
        i32 have = alloc.size();
        const i32 pidx = GetPoolIdx(have);
        auto& ptrs = ms_pools[pidx].ptrs;
        auto& sizes = ms_pools[pidx].sizes;
        if (!sizes.full())
        {
            ptrs.grow() = alloc.begin();
            sizes.grow() = alloc.size();
            alloc = { 0, 0 };
            return true;
        }
        return false;
    }

    static bool Take(i32 want, Allocation& alloc)
    {
        const i32 pidx = GetPoolIdx(want);
        auto& ptrs = ms_pools[pidx].ptrs;
        auto& sizes = ms_pools[pidx].sizes;
        const i32* szs = sizes.begin();
        const i32 ct = sizes.size();
        for (i32 i = 0; i < ct; ++i)
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

    Allocation _Alloc(i32 want)
    {
        DebugAssert(want >= 0);
        Allocation got = { 0, 0 };
        if (want > 0)
        {
            if (!Take(want, got))
            {
                void* ptr = _aligned_malloc(want, 64);
                got = { (u8*)ptr, want };
            }
            DebugAssert(got.begin());
            pimclr(got.begin(), got.size());
        }
        return got;
    }

    void _Realloc(Allocation& prev, i32 want)
    {
        const i32 has = prev.size();
        DebugAssert(want >= 0);
        DebugAssert(has >= 0);

        Allocation got = { 0, 0 };
        if (want <= 0)
        {
            _Free(prev);
        }
        else if (want > has)
        {
            got = _Alloc(want);
            pimcpy(got.begin(), prev.begin(), has);
            _Free(prev);
        }
        else
        {
            got = prev;
        }
        prev = got;
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
