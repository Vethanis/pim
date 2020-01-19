#pragma once

#include "os/thread.h"
#include "common/io.h"
#include "common/heap_item.h"

struct StreamFile
{
    OS::RWLock m_lock;
    IO::FileMap m_map;

    StreamFile() : m_lock(), m_map() {}
    ~StreamFile() { Close(); }

    bool Open(cstr path)
    {
        ASSERT(path);

        EResult err = EUnknown;
        IO::FDGuard file = IO::Open(path, IO::OBinary | IO::OReadWrite, err);
        if (err != ESuccess)
        {
            return false;
        }
        IO::MapGuard map = IO::MapFile(file, true, err);
        if (err != ESuccess)
        {
            return false;
        }

        file.Take();
        m_map = map.Take();
        m_lock.Open();

        return true;
    }

    void Close()
    {
        m_lock.Close();
        IO::Close(m_map);
    }

    bool Read(void* dst, HeapItem src)
    {
        OS::ReadGuard guard(m_lock);
        Slice<u8> mem = m_map.memory;
        if (mem.ValidSlice(src.offset, src.size))
        {
            Slice<u8> memSrc = mem.Subslice(src.offset, src.size);
            memcpy(dst, memSrc.begin(), src.size);
            return true;
        }
        return false;
    }

    bool Write(HeapItem dst, const void* src)
    {
        OS::WriteGuard guard(m_lock);
        Slice<u8> mem = m_map.memory;
        if (mem.ValidSlice(dst.offset, dst.size))
        {
            Slice<u8> memDst = mem.Subslice(dst.offset, dst.size);
            memcpy(memDst.begin(), src, dst.size);
            return true;
        }
        return false;
    }
};
