#include "assets/file_streaming.h"

#include "common/guid.h"
#include "common/guid_util.h"
#include "common/io.h"
#include "common/stringutil.h"
#include "common/text.h"
#include "common/chunk_allocator.h"
#include "components/system.h"
#include "containers/array.h"
#include "containers/heap.h"
#include "os/thread.h"
#include "threading/task_system.h"
#include "threading/task_group.h"

static constexpr u32 kNumRows = 32;
static constexpr u32 kRowMask = kNumRows - 1;

struct StreamFile
{
    OS::RWLock m_lock;
    IO::FileMap m_map;

    bool Open(cstr path)
    {
        ASSERT(path);

        EResult err = EUnknown;
        IO::FDGuard file = IO::Open(path, IO::OBinary | IO::OReadWrite, err);
        if (err != ESuccess)
        {
            return false;
        }
        IO::MapGuard map = IO::MapFile(file.Take(), true, err);
        if (err != ESuccess)
        {
            return false;
        }
        m_map = map.Take();
        m_lock.Open();
        return true;
    }

    void Close()
    {
        m_lock.Close();
        IO::Close(m_map);
    }

    bool Read(void* dst, i32 offset, i32 size)
    {
        bool valid = false;
        m_lock.LockReader();
        Slice<u8> mem = m_map.memory;
        valid = mem.ValidSlice(offset, size);
        if (valid)
        {
            Slice<u8> src = mem.Subslice(offset, size);
            memcpy(dst, src.begin(), size);
        }
        m_lock.UnlockReader();
        return valid;
    }

    bool Write(i32 offset, i32 size, const void* src)
    {
        bool valid = false;
        m_lock.LockWriter();
        Slice<u8> mem = m_map.memory;
        valid = mem.ValidSlice(offset, size);
        if (valid)
        {
            Slice<u8> dst = mem.Subslice(offset, size);
            memcpy(dst.begin(), src, size);
        }
        m_lock.UnlockWriter();
        return valid;
    }
};

struct ChunkStore
{
    OS::Mutex m_lock;
    ChunkAllocator m_allocator;

    void Init()
    {
        m_lock.Open();
        m_allocator.Init(Alloc_Pool, sizeof(StreamFile));
    }

    void Shutdown()
    {
        m_lock.Close();
        m_allocator.Reset();
    }

    StreamFile* Alloc()
    {
        m_lock.Lock();
        StreamFile* ptr = (StreamFile*)m_allocator.Allocate();
        m_lock.Unlock();
        ASSERT(ptr);
        memset(ptr, 0, sizeof(StreamFile));
        return ptr;
    }

    void Free(StreamFile* pFile)
    {
        if (pFile)
        {
            m_lock.Lock();
            m_allocator.Free(pFile);
            m_lock.Unlock();
        }
    }
};

static ChunkStore ms_chunks;

struct PathStore
{
    OS::RWLock m_lock;
    HashDict<Guid, PathText, GuidComparator> m_paths;

    void Init()
    {
        m_lock.Open();
        m_paths.Init(Alloc_Pool);
    }

    void Shutdown()
    {
        m_lock.Close();
        m_paths.Reset();
    }

    bool Get(Guid guid, PathText& dstOut)
    {
        bool acquired = false;
        m_lock.LockReader();
        PathText* pPath = m_paths.Get(guid);
        if (pPath)
        {
            dstOut = *pPath;
            acquired = true;
        }
        m_lock.UnlockReader();
        return acquired;
    }

    void Set(Guid guid, const PathText& srcIn)
    {
        m_lock.LockWriter();
        m_paths[guid] = srcIn;
        m_lock.UnlockWriter();
    }
};

static PathStore ms_paths;

struct Row
{
    OS::RWLock m_lock;
    HashDict<Guid, StreamFile*, GuidComparator> m_files;

    void Init()
    {
        m_lock.Open();
        m_files.Init(Alloc_Pool);
    }

    void Shutdown()
    {
        m_lock.LockWriter();
        for (auto pair : m_files)
        {
            StreamFile* pFile = pair.Value;
            ASSERT(pFile);
            pFile->Close();
        }
        m_lock.UnlockWriter();
        m_lock.Close();
        m_files.Reset();
    }

    StreamFile* Get(Guid guid)
    {
        m_lock.LockReader();
        StreamFile** ppFile = m_files.Get(guid);
        StreamFile* pFile = ppFile ? *ppFile : nullptr;
        m_lock.UnlockReader();
        return pFile;
    }

    bool Add(Guid guid, StreamFile* pFile)
    {
        ASSERT(pFile);
        m_lock.LockWriter();
        bool added = m_files.Add(guid, pFile);
        m_lock.UnlockWriter();
        return added;
    }

    StreamFile* GetAdd(Guid guid)
    {
        StreamFile* pFile = Get(guid);
        if (pFile)
        {
            return pFile;
        }
        PathText path = {};
        if (!ms_paths.Get(guid, path))
        {
            return nullptr;
        }
        pFile = ms_chunks.Alloc();
        if (!pFile->Open(path))
        {
            pFile->Close();
        }
        if (!Add(guid, pFile))
        {
            pFile->Close();
            ms_chunks.Free(pFile);
            pFile = nullptr;
        }
        return pFile;
    }
};

struct Rows
{
    Row m_rows[kNumRows];

    void Init()
    {
        for (Row& row : m_rows)
        {
            row.Init();
        }
    }
    void Shutdown()
    {
        for (Row& row : m_rows)
        {
            row.Shutdown();
        }
    }

    Row& operator[](Guid guid)
    {
        return m_rows[Hash(guid) & kRowMask];
    }

    StreamFile* Get(Guid guid)
    {
        return (*this)[guid].Get(guid);
    }

    bool Add(Guid guid, StreamFile* pFile)
    {
        return (*this)[guid].Add(guid, pFile);
    }

    StreamFile* GetAdd(Guid guid)
    {
        return (*this)[guid].GetAdd(guid);
    }
};

static Rows ms_rows;

void FileTask::Execute(u32, u32, u32)
{
    ASSERT(m_ptr);
    StreamFile* pFile = ms_rows.GetAdd(m_guid);
    if (pFile)
    {
        if (m_write)
        {
            pFile->Write(m_offset, m_size, m_ptr);
        }
        else
        {
            pFile->Read(m_ptr, m_offset, m_size);
        }
    }
}

i32 FileTask::Compare(const CmpType& lhs, const CmpType& rhs)
{
    const FileTask* pLhs = static_cast<const FileTask*>(lhs);
    const FileTask* pRhs = static_cast<const FileTask*>(rhs);
    i32 guidCmp = ::Compare(pLhs->m_guid, pRhs->m_guid);
    if (guidCmp)
    {
        return guidCmp;
    }
    if (pLhs->m_write | pRhs->m_write)
    {
        if (::Overlaps(
            pLhs->m_offset, pLhs->m_offset + pLhs->m_size,
            pRhs->m_offset, pRhs->m_offset + pRhs->m_size))
        {
            return 0;
        }
    }
    return pLhs->m_offset - pRhs->m_offset;
}

namespace FStream
{
    static void Init()
    {
        ms_chunks.Init();
        ms_paths.Init();
        ms_rows.Init();
    }

    static void Update()
    {

    }

    static void Shutdown()
    {
        ms_rows.Shutdown();
        ms_paths.Shutdown();
        ms_chunks.Shutdown();
    }

    static constexpr System ms_system =
    {
        ToGuid("FStream"),
        { 0, 0 },
        Init,
        Update,
        Shutdown,
    };
    static RegisterSystem ms_register(ms_system);
};
