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

void* TlsfAllocator::Alloc(i32 bytes)
{
    using namespace Allocator;

    if (bytes <= 0)
    {
        ASSERT(bytes == 0);
        return nullptr;
    }
    bytes = AlignBytes(bytes);

    if (m_type != Alloc_Task)
    {
        m_lock.Lock();
    }

    Header* hNew = (Header*)tlsf_malloc(m_impl, bytes);

    if (m_type != Alloc_Task)
    {
        m_lock.Unlock();
    }

    return MakePtr(hNew, m_type, bytes);
}

void TlsfAllocator::Free(void* pOld)
{
    using namespace Allocator;

    if (pOld)
    {
        Header* hOld = ToHeader(pOld, m_type);
        ASSERT(Dec(hOld->refcount) == 1);

        if (m_type != Alloc_Task)
        {
            m_lock.Lock();
        }

        tlsf_free(m_impl, hOld);

        if (m_type != Alloc_Task)
        {
            m_lock.Unlock();
        }
    }
}

void* TlsfAllocator::Realloc(void* pOld, i32 bytes)
{
    using namespace Allocator;

    if (!pOld)
    {
        return Alloc(bytes);
    }
    if (bytes <= 0)
    {
        ASSERT(bytes == 0);
        Free(pOld);
        return nullptr;
    }
    Header* hOld = ToHeader(pOld, m_type);
    ASSERT(Load(hOld->refcount) == 1);

    bytes = AlignBytes(bytes);

    if (m_type != Alloc_Task)
    {
        m_lock.Lock();
    }

    Header* hNew = (Header*)tlsf_realloc(m_impl, hOld, bytes);

    if (m_type != Alloc_Task)
    {
        m_lock.Unlock();
    }

    return MakePtr(hNew, m_type, bytes);
}
