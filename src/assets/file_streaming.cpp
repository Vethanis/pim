//#include "assets/file_streaming.h"
//
//#include "common/guid.h"
//#include "common/guid_util.h"
//#include "common/io.h"
//#include "common/stringutil.h"
//#include "common/text.h"
//#include "components/system.h"
//#include "containers/hash_dict.h"
//#include "containers/queue.h"
//#include "containers/array.h"
//#include "containers/heap.h"
//#include "os/thread.h"
//
//struct StreamFile
//{
//    OS::RWLock m_lock;
//    IO::FileMap m_map;
//};
//
//struct StreamQueue
//{
//    OS::Mutex mutex;
//    Queue<i32> queue;
//    Array<i32> freelist;
//    Array<Guid> names;
//    Array<HeapItem> locs;
//    Array<bool> writes;
//    Array<void*> ptrs;
//    Array<EResult*> results;
//};
//
//static StreamQueue ms_queue;
//static HashDict<Guid, StreamFile*, GuidComparator> ms_files;
//
//static constexpr i32 NumStreamThreads = 4;
//static OS::Thread ms_threads[NumStreamThreads];
//static OS::LightSema ms_sema;
//static OS::Mutex ms_queueMutex;
//static OS::Mutex ms_filesMutex;
//static bool ms_run;
//
//static i32 Compare(const i32& lhs, const i32& rhs)
//{
//    {
//        const Guid lname = ms_queue.names[lhs];
//        const Guid rname = ms_queue.names[rhs];
//        const i32 i = Compare(lname, rname);
//        if (i)
//        {
//            return i;
//        }
//    }
//    {
//        const HeapItem lheap = ms_queue.locs[lhs];
//        const HeapItem rheap = ms_queue.locs[rhs];
//        if (Overlaps(lheap, rheap))
//        {
//            const bool lwrite = ms_queue.writes[lhs];
//            const bool rwrite = ms_queue.writes[rhs];
//            if (lwrite || rwrite)
//            {
//                return 0;
//            }
//        }
//        return lheap.offset - rheap.offset;
//    }
//}
//
//static Slice<const u8> GetFile(Guid guid, cstr path)
//{
//    ASSERT(!IsNull(guid));
//    ASSERT(path);
//
//    IO::FileMap file = {};
//
//    ms_filesMutex.Lock();
//    {
//        StreamFile* pFile = ms_files.Get(guid);
//        if (pFile)
//        {
//            file = *pFile;
//        }
//        else
//        {
//            EResult err = EUnknown;
//            IO::FD fd = IO::Open(path, IO::OBinSeqRead, err);
//            if (err == ESuccess)
//            {
//                file = IO::MapFile(fd, false, err);
//            }
//            if (err == ESuccess)
//            {
//                bool added = ms_files.Add(guid, file);
//                ASSERT(added);
//            }
//        }
//    }
//    ms_filesMutex.Unlock();
//
//    return file.memory;
//}
//
//static EResult CopyRegion(Slice<const u8> memory, i32 offset, i32 size, void* dst)
//{
//    if (memory.ValidSlice(offset, size))
//    {
//        Slice<const u8> src = memory.Subslice(offset, size);
//        memcpy(dst, src.begin(), size);
//        return ESuccess;
//    }
//
//    return EFail;
//}
//
//static WorkItem PopItem()
//{
//    // ms_sema should ensure 1:1 work items to Signal()'s
//    WorkItem item = {};
//    ms_queueMutex.Lock();
//    item = ms_queue.Pop();
//    ms_queueMutex.Unlock();
//    return item;
//}
//
//static void PushItem(WorkItem item)
//{
//    ms_queueMutex.Lock();
//    ms_queue.Push(item, { Compare });
//    ms_queueMutex.Unlock();
//    ms_sema.Signal(1);
//}
//
//static void ThreadFn(void*)
//{
//    while (true)
//    {
//        ms_sema.Wait();
//        if (!ms_run)
//        {
//            break;
//        }
//
//        WorkItem item = PopItem();
//        Slice<const u8> memory = GetFile(item.guid, item.path);
//        EResult status = CopyRegion(
//            memory,
//            item.desc.offset,
//            item.desc.size,
//            item.desc.dst);
//        ASSERT(item.desc.status);
//        item.desc.status[0] = status;
//    }
//}
//
//namespace FStream
//{
//    void Request(cstr path, FStreamDesc desc)
//    {
//        ASSERT(path);
//        ASSERT(desc.dst);
//        ASSERT(desc.status);
//
//        desc.status[0] = EUnknown;
//        PushItem({ ToGuid(path), desc, PathText(path) });
//    }
//
//    // ------------------------------------------------------------------------
//
//    static void Init()
//    {
//        ms_files.Init(Alloc_Pool);
//        ms_queue.Init(Alloc_Pool);
//
//        ms_sema.Open(0);
//        ms_filesMutex.Open();
//        ms_queueMutex.Open();
//        ms_run = true;
//
//        for (OS::Thread& thr : ms_threads)
//        {
//            thr.Open(ThreadFn, nullptr);
//        }
//    }
//
//    static void Update()
//    {
//
//    }
//
//    static void Shutdown()
//    {
//        ms_run = false;
//        ms_sema.Signal(NELEM(ms_threads));
//
//        for (OS::Thread& thr : ms_threads)
//        {
//            thr.Join();
//        }
//
//        ms_sema.Close();
//        ms_filesMutex.Close();
//        ms_queueMutex.Close();
//
//        for (auto pair : ms_files)
//        {
//            IO::Close(pair.Value);
//        }
//
//        ms_files.Reset();
//        ms_queue.Reset();
//    }
//
//    static constexpr System ms_system =
//    {
//        ToGuid("FStream"),
//        { 0, 0 },
//        Init,
//        Update,
//        Shutdown,
//    };
//    static RegisterSystem ms_register(ms_system);
//};
