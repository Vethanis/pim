#include "assets/file_streaming.h"

#include "common/chunk_allocator.h"
#include "common/guid_util.h"
#include "common/io.h"
#include "components/system.h"
#include "containers/array.h"
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
        IO::MapGuard map = IO::MapFile(file, true, err);
        if (err != ESuccess)
        {
            return false;
        }

        file.Take();
        m_map = map.Take();
        m_lock.Open();

        return true;
    }

    void Close()
    {
        m_lock.Close();
        IO::Close(m_map);
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
};
static ChunkAllocator ms_streamFilePool;

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

        pFile = (StreamFile*)ms_streamFilePool.Allocate();

        if (!pFile->Open(path))
        {
            pFile->Close();
            ms_streamFilePool.Free(pFile);
            return nullptr;
        }

        if (!Add(guid, pFile))
        {
            pFile->Close();
            ms_streamFilePool.Free(pFile);
            return Get(guid);
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
static ObjPool<FileTask> ms_taskPool;

void FileTask::Execute(u32 tid)
{
    StreamFile* pFile = ms_rows.GetAdd(path);
    if (!pFile)
    {
        if (OnError)
        {
            OnError(*this);
        }
        return;
    }
    if (write)
    {
        pFile->Write(pos, ptr);
    }
    else
    {
        pFile->Read(ptr, pos);
    }
    if (OnSuccess)
    {
        OnSuccess(*this);
    }
    ms_taskPool.Delete(this);
}

namespace FStream
{
    FileTask* Read(cstr path, void* dst, HeapItem src)
    {
        ASSERT(path);
        ASSERT(dst);
        FileTask* pTask = ms_taskPool.New();
        pTask->path = path;
        pTask->ptr = dst;
        pTask->pos = src;
        pTask->write = false;
        pTask->OnSuccess = nullptr;
        pTask->OnError = nullptr;
        return pTask;
    }

    FileTask* Write(cstr path, HeapItem dst, void* src)
    {
        ASSERT(path);
        ASSERT(src);
        FileTask* pTask = ms_taskPool.New();
        pTask->path = path;
        pTask->ptr = src;
        pTask->pos = dst;
        pTask->write = true;
        pTask->OnSuccess = nullptr;
        pTask->OnError = nullptr;
        return pTask;
    }

    static void Init()
    {
        ms_streamFilePool.Init(Alloc_Pool, sizeof(StreamFile));
        ms_taskPool.Init();
        ms_rows.Init();
    }

    static void Update()
    {

    }

    static void Shutdown()
    {
        ms_rows.Shutdown();
        ms_taskPool.Shutdown();
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
