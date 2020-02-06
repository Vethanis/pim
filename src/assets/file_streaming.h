
#include "common/int_types.h"
#include "common/guid.h"
#include "common/heap_item.h"
#include "common/text.h"
#include "threading/task.h"

struct FileTask
{
    Task m_base;
    PathText m_path;
    HeapItem m_pos;
    void* m_ptr;
    bool m_write;
    bool m_success;

    void Init(cstr path, HeapItem pos, void* ptr, bool write)
    {
        m_base.InitAs<FileTask>();
        m_path = PathText(path);
        m_pos = pos;
        m_ptr = ptr;
        m_write = write;
        m_success = false;
    }

    void InitRead(cstr path, void* dst, HeapItem src)
    {
        Init(path, src, dst, false);
    }

    void InitWrite(cstr path, HeapItem dst, void* src)
    {
        Init(path, dst, src, true);
    }

    void Execute();
};

namespace FStream
{
    FileTask Read(cstr path, void* dst, HeapItem src);
    FileTask Write(cstr path, HeapItem dst, void* src);
};
