#pragma once

#include "threading/task.h"
#include "common/text.h"
#include "containers/slice.h"
#include "allocator/allocator.h"

namespace StreamFile
{
    EResult Read(cstr path, Slice<u8>& dst, i32 offset, i32 length);
    EResult Write(cstr path, Slice<const u8> src, i32 offset);
};

struct ReadTask : ITask
{
    ReadTask() : ITask(0, 1, 1) {}

    void Setup(cstr path, i32 offset, i32 length)
    {
        ASSERT(path);
        ASSERT(offset >= 0);
        ASSERT((length > 0) || (length == -1));

        m_path = path;
        m_offset = offset;
        m_length = length;
    }

    cstr GetPath() const { return m_path; }
    i32 GetOffset() const { return m_offset; }
    i32 GetLength() const { return m_length; }

    virtual void OnResult(EResult err, Slice<u8> value) = 0;
    void Execute(i32, i32) final
    {
        Slice<u8> dst = {};
        EResult err = StreamFile::Read(m_path, dst, m_offset, m_length);
        OnResult(err, dst);
    }

private:
    PathText m_path;
    i32 m_offset;
    i32 m_length;
};

struct WriteTask : ITask
{
    WriteTask() : ITask(0, 1, 1) {}

    void Setup(cstr path, i32 offset, Slice<const u8> src)
    {
        ASSERT(path);
        ASSERT(offset >= 0);
        ASSERT(src.begin());
        ASSERT(src.size() > 0);

        m_path = path;
        m_offset = offset;
        m_src = src;
    }

    cstr GetPath() const { return m_path; }
    i32 GetOffset() const { return m_offset; }
    i32 GetLength() const { return m_src.size(); }

    virtual void OnResult(EResult err) = 0;
    void Execute(i32, i32) final
    {
        EResult err = StreamFile::Write(m_path, m_src, m_offset);
        OnResult(err);
    }

private:
    PathText m_path;
    i32 m_offset;
    Slice<const u8> m_src;
};
