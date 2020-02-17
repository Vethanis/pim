#pragma once

#include "threading/task.h"
#include "os/thread.h"
#include "containers/mtqueue.h"

struct StreamFileArgs
{
    cstr path;
};

struct StreamFile final : ITask
{
    StreamFile(StreamFileArgs args);
    ~StreamFile();

    bool IsOpen() const;
    bool AddRead(void* dst, i32 offset, i32 size, i32* pCompleted);
    bool AddWrite(i32 offset, i32 size, void* src, i32* pCompleted);
    void Execute(i32 begin, i32 end) final;

private:

    struct FileOp
    {
        void* ptr;
        i32 offset;
        i32 size;
        i32* pCompleted;
        bool write;

        bool operator<(const FileOp& rhs) const;
    };

    OS::RWLock m_lock;
    void* m_file;
    MtQueue<FileOp> m_queue;
};
