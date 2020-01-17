
#include "common/int_types.h"
#include "common/guid.h"
#include "common/heap_item.h"
#include "common/text.h"
#include "threading/task_system.h"

struct FileTask final : ITask
{
    PathText path;
    HeapItem pos;
    void* ptr;
    bool write;
    void(*OnSuccess)(FileTask& task);
    void(*OnError)(FileTask& task);

    void Execute(u32 tid) final;
};

namespace FStream
{
    FileTask* Read(cstr path, void* dst, HeapItem src);
    FileTask* Write(cstr path, HeapItem dst, void* src);
};
