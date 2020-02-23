#define _CRT_SECURE_NO_WARNINGS 1

#include "assets/stream_file.h"
#include "common/minmax.h"
#include "common/sort.h"
#include "containers/array.h"
#include "os/atomics.h"
#include <stdio.h>

bool StreamFile::FileOp::operator<(const StreamFile::FileOp& rhs) const
{
    if (write | rhs.write)
    {
        if (::Overlaps(offset, offset + size, rhs.offset, rhs.offset + rhs.size))
        {
            return 0;
        }
    }
    return offset - rhs.offset;
}

StreamFile::StreamFile(StreamFileArgs args) : ITask(0, 1, 1)
{
    ASSERT(args.path);
    m_lock.Open();
    m_file = fopen(args.path, "r+b");
    m_queue.Init(Alloc_Perm, 64);
}

StreamFile::~StreamFile()
{
    m_lock.LockWriter();
    if (m_file)
    {
        fclose((FILE*)m_file);
        m_file = 0;
    }
    m_queue.Reset();
    m_lock.Close();
}

bool StreamFile::IsOpen() const
{
    OS::ReadGuard guard(m_lock);
    return m_file != 0;
}

bool StreamFile::AddRead(void* dst, i32 offset, i32 size, i32* pCompleted)
{
    ASSERT(dst);
    ASSERT(offset >= 0);
    ASSERT(size >= 0);
    ASSERT(pCompleted);

    if (!IsOpen())
    {
        return false;
    }

    Store(*pCompleted, 0);

    FileOp op;
    op.ptr = dst;
    op.offset = offset;
    op.size = size;
    op.pCompleted = pCompleted;
    op.write = false;
    m_queue.Push(op);

    if (IsInitOrComplete())
    {
        TaskSystem::Submit(this);
    }
    return true;
}

bool StreamFile::AddWrite(i32 offset, i32 size, void* src, i32* pCompleted)
{
    ASSERT(src);
    ASSERT(offset >= 0);
    ASSERT(size >= 0);
    ASSERT(pCompleted);

    if (!IsOpen())
    {
        return false;
    }

    Store(*pCompleted, 0);

    FileOp op;
    op.ptr = src;
    op.offset = offset;
    op.size = size;
    op.pCompleted = pCompleted;
    op.write = true;
    m_queue.Push(op);

    if (IsInitOrComplete())
    {
        TaskSystem::Submit(this);
    }
    return true;
}

void StreamFile::Execute(i32 begin, i32 end)
{
    if (!IsOpen() || !m_queue.size())
    {
        return;
    }

    Array<FileOp> ops = CreateArray<FileOp>(Alloc_Task, m_queue.size());
    {
        FileOp op = {};
        while (m_queue.TryPop(op))
        {
            ops.PushBack(op);
        }
    }

    Sort(ops.begin(), ops.size());

    OS::WriteGuard guard(m_lock);
    FILE* file = (FILE*)m_file;

    if (file)
    {
        for (FileOp& op : ops)
        {
            i32 rval = fseek(file, op.offset, SEEK_SET);
            ASSERT(!rval);

            usize ct = 0;
            if (op.write)
            {
                ct = fwrite(op.ptr, 1, op.size, file);
            }
            else
            {
                ct = fread(op.ptr, 1, op.size, file);
            }
            ASSERT(ct == (usize)op.size);
            Store(*op.pCompleted, 1);
        }
    }
    else
    {
        for (FileOp& op : ops)
        {
            Store(*op.pCompleted, -1);
        }
    }

    ops.Reset();
}
