#include "assets/file_streaming.h"

#include "common/guid.h"
#include "common/guid_util.h"
#include "common/io.h"
#include "common/stringutil.h"
#include "common/text.h"
#include "components/system.h"
#include "containers/hash_dict.h"
#include "containers/queue.h"
#include "containers/array.h"
#include "containers/heap.h"
#include "os/thread.h"

struct StreamFile
{
    OS::RWLock m_lock;
    IO::FileMap m_map;
};

struct StreamQueue
{
    OS::Mutex mutex;
    Queue<i32> queue;
    Array<i32> freelist;
    Array<Guid> names;
    Array<HeapItem> locs;
    Array<bool> writes;
    Array<void*> ptrs;
    Array<EResult*> results;
};

static StreamQueue ms_queue;
static HashDict<Guid, StreamFile*, GuidComparator> ms_files;

static i32 Compare(const i32& lhs, const i32& rhs)
{
    {
        const Guid lname = ms_queue.names[lhs];
        const Guid rname = ms_queue.names[rhs];
        const i32 i = Compare(lname, rname);
        if (i)
        {
            return i;
        }
    }
    {
        const HeapItem lheap = ms_queue.locs[lhs];
        const HeapItem rheap = ms_queue.locs[rhs];
        if (Overlaps(lheap, rheap))
        {
            const bool lwrite = ms_queue.writes[lhs];
            const bool rwrite = ms_queue.writes[rhs];
            if (lwrite || rwrite)
            {
                return 0;
            }
        }
        return lheap.offset - rheap.offset;
    }
}


namespace FStream
{
    void Request(cstr path, FStreamDesc desc)
    {

    }

    // ------------------------------------------------------------------------

    static void Init()
    {
    }

    static void Update()
    {

    }

    static void Shutdown()
    {
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
