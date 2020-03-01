#include "allocator/tlsf_allocator.h"

#include "common/macro.h"
#include "allocator/header.h"
#include "tlsf/tlsf.h"
#include <stdlib.h>

void TlsfAllocator::Init(i32 bytes, AllocType type)
{
    if (type != Alloc_Task)
    {
        m_lock.Open();
    }

    m_bytes = bytes;
    m_type = type;
    m_memory = malloc(bytes);
    m_impl = tlsf_create_with_pool(m_memory, bytes);
}

void TlsfAllocator::Reset()
{
    if (m_type != Alloc_Task)
    {
        m_lock.Lock();
    }

    tlsf_destroy(m_impl);
    free(m_memory);
    m_impl = 0;
    m_memory = 0;
    m_lock.Close();
}

void TlsfAllocator::Clear()
{
    if (m_type != Alloc_Task)
    {
        m_lock.Lock();
    }

    tlsf_destroy(m_impl);
    m_impl = tlsf_create_with_pool(m_memory, m_bytes);

    if (m_type != Alloc_Task)
    {
        m_lock.Unlock();
    }
}

void* TlsfAllocator::Alloc(i32 userBytes)
{
    using namespace Allocator;

    if (userBytes <= 0)
    {
        ASSERT(userBytes == 0);
        return nullptr;
    }
    const i32 rawBytes = AlignBytes(userBytes);

    if (m_type != Alloc_Task)
    {
        m_lock.Lock();
    }

    void* pRaw = tlsf_malloc(m_impl, rawBytes);

    if (m_type != Alloc_Task)
    {
        m_lock.Unlock();
    }

    return RawToUser(pRaw, m_type, rawBytes);
}

void TlsfAllocator::Free(void* pUser)
{
    using namespace Allocator;

    if (pUser)
    {
        void* pRaw = UserToRaw(pUser, m_type);

        if (m_type != Alloc_Task)
        {
            m_lock.Lock();
        }

        tlsf_free(m_impl, pRaw);

        if (m_type != Alloc_Task)
        {
            m_lock.Unlock();
        }
    }
}

void* TlsfAllocator::Realloc(void* pOldUser, i32 userBytes)
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

    void* pNewUser = Alloc(userBytes);
    ASSERT(pNewUser);

    Copy(UserToHeader(pNewUser, m_type), UserToHeader(pOldUser, m_type));
    Free(pOldUser);

    return pNewUser;
}
