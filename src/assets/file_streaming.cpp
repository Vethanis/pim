#include "assets/file_streaming.h"

#include "assets/stream_file.h"
#include "containers/obj_table.h"
#include "common/guid_util.h"
#include "components/system.h"

using FileTable = ObjTable<Guid, StreamFile, GuidComparator>;

static FileTable ms_table;

static StreamFile* GetAdd(cstr path)
{
    Guid key = ToGuid(path);
    StreamFile* pFile = ms_table.Get(key);
    if (pFile)
    {
        return pFile;
    }
    pFile = ms_table.New();
    if (pFile->Open(path))
    {
        if (ms_table.Add(key, pFile))
        {
            return pFile;
        }
    }
    ms_table.Delete(pFile);
    return ms_table.Get(key);
}

void FileTask::Execute()
{
    m_success = false;
    StreamFile* pFile = GetAdd(m_path);
    if (pFile)
    {
        if (m_write)
        {
            m_success = pFile->Write(m_pos, m_ptr);
        }
        else
        {
            m_success = pFile->Read(m_ptr, m_pos);
        }
    }
}

namespace FStream
{
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
