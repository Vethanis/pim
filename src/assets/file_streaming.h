
#include "common/int_types.h"
#include "common/heap_item.h"

struct FileOp
{
    HeapItem pos;
    void* ptr;
    bool write;
    void(*OnComplete)(FileOp& op, bool success);
    void* userData;
};

namespace FStream
{
    bool Request(cstr path, FileOp op);
};
