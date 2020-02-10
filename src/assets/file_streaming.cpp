#include "assets/file_streaming.h"

#include "assets/stream_file.h"
#include "containers/obj_table.h"
#include "common/guid.h"
#include "components/system.h"

using FileTable = ObjTable<Guid, StreamFile, StreamFileArgs>;

static FileTable ms_table;

static StreamFile* GetFile(cstr path)
{
    Guid id = ToGuid(path);
    StreamFile* pFile = ms_table.GetAdd(id, { path });
    if (pFile->IsOpen())
    {
        return pFile;
    }
    return nullptr;
}

namespace FStream
{
    bool Submit(FileStreamOp* pOps, i32 count)
    {
        ASSERT(pOps);
        ASSERT(count >= 0);

        for (i32 i = 0; i < count; ++i)
        {
            if (!GetFile(pOps[i].path))
            {
                return false;
            }
        }

        Guid lastId = {};
        StreamFile* pFile = nullptr;
        for (i32 i = 0; i < count; ++i)
        {
            FileStreamOp op = pOps[i];
            Guid id = ToGuid(op.path);
            if (id != lastId)
            {
                lastId = id;
                pFile = ms_table.Get(id);
            }
            ASSERT(pFile);
            bool added = false;
            if (op.write)
            {
                added = pFile->AddWrite(op.offset, op.size, op.ptr, op.pCompleted);
            }
            else
            {
                added = pFile->AddRead(op.ptr, op.offset, op.size, op.pCompleted);
            }
            ASSERT(added);
        }

        return true;
    }

    bool AddRead(cstr path, void* dst, i32 offset, i32 size, i32* pCompleted)
    {
        StreamFile* pFile = GetFile(path);
        if (pFile)
        {
            return pFile->AddRead(dst, offset, size, pCompleted);
        }
        return false;
    }

    bool AddWrite(cstr path, i32 offset, i32 size, void* src, i32* pCompleted)
    {
        StreamFile* pFile = GetFile(path);
        if (pFile)
        {
            return pFile->AddWrite(offset, size, src, pCompleted);
        }
        return false;
    }

    static void Init()
    {
        ms_table.Init();
    }

    static void Update()
    {

    }

    static void Shutdown()
    {
        ms_table.Reset();
    }

    DEFINE_SYSTEM("FStream", {}, Init, Update, Shutdown)
};
