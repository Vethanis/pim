
#include "common/int_types.h"
#include "common/guid.h"
#include "common/heap_item.h"
#include "common/text.h"
#include "threading/task.h"

struct FileTask final : ITask
{
    PathText path;
    HeapItem pos;
    void* ptr;
    bool write;
    bool success;

    void Execute(u32 tid) final;
};

namespace FStream
{
    FileTask* Read(cstr path, void* dst, HeapItem src);
    FileTask* Write(cstr path, HeapItem dst, void* src);
    void Free(FileTask* pTask);
};
