
#include "common/int_types.h"
#include "common/text.h"

struct FileStreamOp
{
    PathText path;
    void* ptr;
    i32 offset;
    i32 size;
    i32* pCompleted;
    bool write;
};

namespace FStream
{
    bool Submit(FileStreamOp* pOps, i32 count);
    bool AddRead(cstr path, void* dst, i32 offset, i32 size, i32* pCompleted);
    bool AddWrite(cstr path, i32 offset, i32 size, void* src, i32* pCompleted);
};
