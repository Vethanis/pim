#include "allocator/tlsf_allocator.h"

#include "common/macro.h"
#include "allocator/header.h"
#include "tlsf/tlsf.h"
#include <stdlib.h>

TlsfAllocator::TlsfAllocator(i32 bytes) : IAllocator(bytes)
{
    m_lock.Open();
    m_bytes = bytes;
    m_memory = malloc(bytes);
    m_impl = tlsf_create_with_pool(m_memory, bytes);
}

TlsfAllocator::~TlsfAllocator()
{
    m_lock.Lock();
    tlsf_destroy(m_impl);
    free(m_memory);
    m_impl = 0;
    m_memory = 0;
    m_lock.Close();
}

void TlsfAllocator::Clear()
{
    OS::LockGuard guard(m_lock);
    tlsf_destroy(m_impl);
    m_impl = tlsf_create_with_pool(m_memory, m_bytes);
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
    OS::LockGuard guard(m_lock);
    Header* hNew = (Header*)tlsf_malloc(m_impl, bytes);
    return MakePtr(hNew, Alloc_Tlsf, bytes);
}

void TlsfAllocator::Free(void* pOld)
{
    using namespace Allocator;

    if (pOld)
    {
        Header* hOld = ToHeader(pOld, Alloc_Tlsf);
        const i32 rc = Dec(hOld->refcount);
        ASSERT(rc == 1);

        OS::LockGuard guard(m_lock);
        tlsf_free(m_impl, hOld);
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
    Header* hOld = ToHeader(pOld, Alloc_Tlsf);
    const i32 rc = Load(hOld->refcount);
    ASSERT(rc == 1);

    bytes = AlignBytes(bytes);
    OS::LockGuard guard(m_lock);
    Header* hNew = (Header*)tlsf_realloc(m_impl, hOld, bytes);
    return MakePtr(hNew, Alloc_Tlsf, bytes);
}
