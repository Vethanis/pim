#include "assets/file_streaming.h"

#include "common/chunk_allocator.h"
#include "common/guid.h"
#include "common/guid_util.h"
#include "common/io.h"
#include "common/sort.h"
#include "common/text.h"
#include "components/system.h"
#include "containers/array.h"
#include "containers/heap.h"
#include "os/thread.h"
#include "threading/task_system.h"
#include <new>

template<typename T>
struct ObjPool
{
    ChunkAllocator m_chunks;

    void Init()
    {
        m_chunks.Init(Alloc_Pool, sizeof(T));
    }
    void Shutdown()
    {
        m_chunks.Shutdown();
    }
    T* New()
    {
        void* pMem = m_chunks.Allocate();
        return new (pMem) T();
    }
    void Delete(T* ptr)
    {
        if (ptr)
        {
            ptr->~T();
            m_chunks.Free(ptr);
        }
    }
};

static i32 Compare(const FileOp& lhs, const FileOp& rhs)
{
    if (lhs.write | rhs.write)
    {
        if (Overlaps(lhs.pos, rhs.pos))
        {
            return 0;
        }
    }
    return lhs.pos.offset - rhs.pos.offset;
}

struct StreamFile : ITask
{
    OS::RWLock m_lock;
    OS::Mutex m_opsLock;
    OS::Mutex m_submitLock;
    IO::FileMap m_map;
    Queue<FileOp> m_ops;

    bool Open(cstr path)
    {
        ASSERT(path);

        EResult err = EUnknown;
        IO::FDGuard file = IO::Open(path, IO::OBinary | IO::OReadWrite, err);
        if (err != ESuccess)
        {
            return false;
        }
        IO::MapGuard map = IO::MapFile(file, true, err);
        if (err != ESuccess)
        {
            return false;
        }

        file.Take();
        m_map = map.Take();

        m_lock.Open();
        m_opsLock.Open();
        m_submitLock.Open();

        m_ops.Init(Alloc_Pool);

        return true;
    }

    void Close()
    {
        if (!IsComplete())
        {
            TaskSystem::Await(this, TaskPriority_Low);
        }

        m_lock.Close();
        m_opsLock.Close();
        m_submitLock.Close();

        IO::Close(m_map);
        m_ops.Reset();
    }

    void Push(FileOp op)
    {
        {
            OS::LockGuard guard(m_opsLock);
            m_ops.Push(op, { Compare });
        }
        if (m_submitLock.TryLock())
        {
            if (IsComplete())
            {
                constexpr i32 kSplitSize = 16;
                constexpr i32 kNumThreads = 4;
                m_taskSize = kSplitSize * kNumThreads;
                m_granularity = 1;
                TaskSystem::Submit(this);
            }
            m_submitLock.Unlock();
        }
    }

    bool TryPop(FileOp& dst)
    {
        OS::LockGuard guard(m_opsLock);
        if (m_ops.HasItems())
        {
            dst = m_ops.Pop();
            return true;
        }
        return false;
    }

    bool Read(void* dst, HeapItem src)
    {
        OS::ReadGuard guard(m_lock);
        Slice<u8> mem = m_map.memory;
        if (mem.ValidSlice(src.offset, src.size))
        {
            Slice<u8> memSrc = mem.Subslice(src.offset, src.size);
            memcpy(dst, memSrc.begin(), src.size);
            return true;
        }
        return false;
    }

    bool Write(HeapItem dst, const void* src)
    {
        OS::WriteGuard guard(m_lock);
        Slice<u8> mem = m_map.memory;
        if (mem.ValidSlice(dst.offset, dst.size))
        {
            Slice<u8> memDst = mem.Subslice(dst.offset, dst.size);
            memcpy(memDst.begin(), src, dst.size);
            return true;
        }
        return false;
    }

    void Execute(u32, u32, u32) final
    {
        FileOp op = {};
        while (TryPop(op))
        {
            bool success = false;
            if (op.write)
            {
                success = Write(op.pos, op.ptr);
            }
            else
            {
                success = Read(op.ptr, op.pos);
            }
            if (op.OnComplete)
            {
                op.OnComplete(op, success);
            }
        }
    }
};
static ObjPool<StreamFile> ms_streamFilePool;

struct Row
{
    OS::RWLock m_fileLock;
    HashDict<Guid, StreamFile*, GuidComparator> m_files;

    void Init()
    {
        m_fileLock.Open();
        m_files.Init(Alloc_Pool);
    }

    void Shutdown()
    {
        m_fileLock.LockWriter();
        for (auto pair : m_files)
        {
            StreamFile* pFile = pair.Value;
            ASSERT(pFile);
            pFile->Close();
        }
        m_files.Reset();
        m_fileLock.UnlockWriter();
        m_fileLock.Close();
    }

    StreamFile* Get(Guid guid)
    {
        m_fileLock.LockReader();
        StreamFile** ppFile = m_files.Get(guid);
        StreamFile* pFile = ppFile ? *ppFile : nullptr;
        m_fileLock.UnlockReader();
        return pFile;
    }

    bool Add(Guid guid, StreamFile* pFile)
    {
        ASSERT(pFile);
        m_fileLock.LockWriter();
        bool added = m_files.Add(guid, pFile);
        m_fileLock.UnlockWriter();
        return added;
    }

    StreamFile* GetAdd(Guid guid, cstr path)
    {
        StreamFile* pFile = Get(guid);
        if (pFile)
        {
            return pFile;
        }

        pFile = ms_streamFilePool.New();

        if (!pFile->Open(path))
        {
            pFile->Close();
            ms_streamFilePool.Delete(pFile);
            return nullptr;
        }

        if (!Add(guid, pFile))
        {
            pFile->Close();
            ms_streamFilePool.Delete(pFile);
            return nullptr;
        }

        return pFile;
    }
};

struct Rows
{
    static constexpr u32 kNumRows = 64;
    static constexpr u32 kRowMask = kNumRows - 1;

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

    StreamFile* GetAdd(cstr path)
    {
        Guid guid = ToGuid(path);
        return m_rows[Hash(guid) & kRowMask].GetAdd(guid, path);
    }
};
static Rows ms_rows;

namespace FStream
{
    bool Request(cstr path, FileOp op)
    {
        ASSERT(path);
        ASSERT(op.ptr);

        StreamFile* pFile = ms_rows.GetAdd(path);
        if (!pFile)
        {
            return false;
        }
        pFile->Push(op);

        return true;
    }

    static void Init()
    {
        ms_streamFilePool.Init();
        ms_rows.Init();
    }

    static void Update()
    {

    }

    static void Shutdown()
    {
        ms_rows.Shutdown();
        ms_streamFilePool.Shutdown();
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
