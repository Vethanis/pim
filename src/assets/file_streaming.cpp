#include "assets/file_streaming.h"

#include "common/guid.h"
#include "common/io.h"
#include "common/stringutil.h"
#include "common/text.h"
#include "components/system.h"
#include "containers/hash_dict.h"
#include "containers/queue.h"
#include "os/thread.h"

static constexpr i32 NumStreamThreads = 8;

using PathText = Text<PIM_PATH>;
static constexpr auto GuidComparator = OpComparator<Guid>();

struct WorkItem
{
    Guid guid;
    FStreamDesc desc;
    PathText path;

    inline static i32 Compare(const WorkItem& lhs, const WorkItem& rhs)
    {
        if (!(lhs.guid == rhs.guid))
        {
            return lhs.guid < rhs.guid ? -1 : 1;
        }
        return lhs.desc.offset - rhs.desc.offset;
    }
};

static HashDict<Guid, IO::FileMap, GuidComparator> ms_files;
static Queue<WorkItem> ms_queue;

static OS::Thread ms_threads[NumStreamThreads];
static OS::Semaphore ms_sema;
static OS::Mutex ms_queueMutex;
static OS::Mutex ms_filesMutex;
static bool ms_run;

static Slice<const u8> GetFile(Guid guid, cstr path)
{
    ASSERT(!IsNull(guid));
    ASSERT(path);

    IO::FileMap file = {};

    ms_filesMutex.Lock();
    {
        IO::FileMap* pFile = ms_files.Get(guid);
        if (pFile)
        {
            file = *pFile;
        }
        else
        {
            file = IO::MapFile(path, false);
            if (IO::IsOpen(file))
            {
                bool added = ms_files.Add(guid, file);
                ASSERT(added);
            }
        }
    }
    ms_filesMutex.Unlock();

    return file.AsSlice();
}

static EResult CopyRegion(Slice<const u8> memory, i32 offset, i32 size, void* dst)
{
    if (memory.ValidSlice(offset, size))
    {
        Slice<const u8> src = memory.Subslice(offset, size);
        memcpy(dst, src.begin(), size);
        return ESuccess;
    }

    return EFail;
}

static WorkItem PopItem()
{
    // ms_sema should ensure 1:1 work items to Signal()'s
    WorkItem item = {};
    ms_queueMutex.Lock();
    item = ms_queue.Pop();
    ms_queueMutex.Unlock();
    return item;
}

static void PushItem(WorkItem item)
{
    ms_queueMutex.Lock();
    ms_queue.Push(item, { WorkItem::Compare });
    ms_queueMutex.Unlock();
    ms_sema.Signal(1);
}

static void ThreadFn(void*)
{
    while (ms_run)
    {
        ms_sema.Wait();
        if (!ms_run)
        {
            break;
        }

        WorkItem item = PopItem();
        Slice<const u8> memory = GetFile(item.guid, item.path);
        EResult status = CopyRegion(
            memory,
            item.desc.offset,
            item.desc.size,
            item.desc.dst);
        ASSERT(item.desc.status);
        item.desc.status[0] = status;
    }
}

namespace FStream
{
    void Request(cstr path, FStreamDesc desc)
    {
        ASSERT(path);
        ASSERT(desc.dst);
        ASSERT(desc.status);

        desc.status[0] = EUnknown;
        PushItem({ ToGuid(path), desc, PathText(path) });
    }

    // ------------------------------------------------------------------------

    static void Init()
    {
        ms_files.Init(Alloc_Pool);
        ms_queue.Init(Alloc_Pool);

        ms_sema.Open(0);
        ms_filesMutex.Open();
        ms_queueMutex.Open();
        ms_run = true;

        for (OS::Thread& thr : ms_threads)
        {
            thr.Open(ThreadFn, nullptr);
        }
    }

    static void Update()
    {

    }

    static void Shutdown()
    {
        ms_run = false;
        ms_sema.Signal(NELEM(ms_threads));

        for (OS::Thread& thr : ms_threads)
        {
            thr.Join();
        }

        ms_sema.Close();
        ms_filesMutex.Close();
        ms_queueMutex.Close();

        for (auto pair : ms_files)
        {
            IO::Close(pair.Value);
        }

        ms_files.Reset();
        ms_queue.Reset();
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
