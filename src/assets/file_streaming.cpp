#include "assets/file_streaming.h"

#include "assets/stream_file.h"
#include "containers/obj_table.h"
#include "common/guid_util.h"
#include "components/system.h"

using FileTable = ObjTable<Guid, StreamFile, GuidComparator, 64>;

static FileTable ms_table;
static ObjPool<FileTask> ms_taskPool;

static StreamFile* GetAdd(cstr path)
{
    Guid key = ToGuid(path);
    StreamFile* pFile = ms_table.Get(key);
    if (pFile)
    {
        return pFile;
    }
    pFile = ms_table.New();
    if (pFile->Open(path))
    {
        if (ms_table.Add(key, pFile))
        {
            return pFile;
        }
    }
    ms_table.Delete(pFile);
    return ms_table.Get(key);
}

void FileTask::Execute(u32 tid)
{
    success = false;
    StreamFile* pFile = GetAdd(path);
    if (pFile)
    {
        if (write)
        {
            success = pFile->Write(pos, ptr);
        }
        else
        {
            success = pFile->Read(ptr, pos);
        }
    }
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
        return pTask;
    }

    void Free(FileTask* pTask)
    {
        ms_taskPool.Delete(pTask);
    }

    static void Init()
    {
        ms_table.Init();
        ms_taskPool.Init();
    }

    static void Update()
    {

    }

    static void Shutdown()
    {
        ms_table.Reset();
        ms_taskPool.Reset();
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
