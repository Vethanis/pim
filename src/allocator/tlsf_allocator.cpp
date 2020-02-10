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
    void* ptr = tlsf_malloc(m_impl, bytes);
    return MakePtr(ptr, Alloc_Tlsf, bytes);
}

void TlsfAllocator::Free(void* ptr)
{
    using namespace Allocator;

    if (ptr)
    {
        Header* hdr = ToHeader(ptr, Alloc_Tlsf);
        const i32 rc = Dec(hdr->refcount);
        ASSERT(rc == 1);

        OS::LockGuard guard(m_lock);
        tlsf_free(m_impl, hdr);
    }
}

void* TlsfAllocator::Realloc(void* prev, i32 bytes)
{
    using namespace Allocator;

    if (!prev)
    {
        return Alloc(bytes);
    }
    if (bytes <= 0)
    {
        ASSERT(bytes == 0);
        Free(prev);
        return nullptr;
    }
    Header* hdr = ToHeader(prev, Alloc_Tlsf);
    const i32 rc = Load(hdr->refcount);
    ASSERT(rc == 1);

    bytes = AlignBytes(bytes);
    OS::LockGuard guard(m_lock);
    void* ptr = tlsf_realloc(m_impl, hdr, bytes);
    return MakePtr(ptr, Alloc_Tlsf, bytes);
}
