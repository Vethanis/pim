#pragma once

#include "threading/task.h"
#include "os/thread.h"
#include "containers/mtqueue.h"

struct StreamFile final : ITask
{
    StreamFile();
    ~StreamFile();

    bool IsOpen() const;
    bool Open(cstr path);
    bool Close();
    bool AddRead(void* dst, i32 offset, i32 size, i32* pCompleted);
    bool AddWrite(i32 offset, i32 size, void* src, i32* pCompleted);
    void Execute() final;

private:

    struct FileOp
    {
        void* ptr;
        i32 offset;
        i32 size;
        i32* pCompleted;
        bool write;
    };

    static i32 Compare(const FileOp& lhs, const FileOp& rhs);

    OS::RWLock m_lock;
    void* m_file;
    MtQueue<FileOp> m_queue;
};
