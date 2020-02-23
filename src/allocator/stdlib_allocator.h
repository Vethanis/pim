#pragma once

#include "allocator/iallocator.h"
#include "allocator/header.h"
#include <stdlib.h>

struct StdlibAllocator final : IAllocator
{
private:
    AllocType m_type;

public:
    void Init(i32 bytes, AllocType type) final { m_type = type; }
    void Reset() final { }
    void Clear() final { }

    void* Alloc(i32 userBytes) final
    {
        using namespace Allocator;

        if (userBytes > 0)
        {
            const i32 rawBytes = AlignBytes(userBytes);
            void* pRaw = malloc(rawBytes);
            return RawToUser(pRaw, m_type, rawBytes);
        }
        ASSERT(userBytes == 0);
        return nullptr;
    }

    void* Realloc(void* pOldUser, i32 userBytes) final
    {
        using namespace Allocator;

        if (!pOldUser)
        {
            return Alloc(userBytes);
        }
        if (userBytes <= 0)
        {
            ASSERT(userBytes == 0);
            Free(pOldUser);
            return nullptr;
        }

        const i32 rawBytes = AlignBytes(userBytes);
        void* pOldRaw = UserToRaw(pOldUser, m_type);

        void* pNewRaw = realloc(pOldRaw, rawBytes);

        return RawToUser(pNewRaw, m_type, rawBytes);
    }

    void Free(void* pUser) final
    {
        using namespace Allocator;

        if (pUser)
        {
            void* pRaw = UserToRaw(pUser, m_type);
            free(pRaw);
        }
    }
};
