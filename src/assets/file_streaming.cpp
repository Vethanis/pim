#include "assets/file_streaming.h"

#include "common/io.h"
#include "common/guid.h"
#include "common/stringutil.h"
#include "components/system.h"
#include "containers/hash_dict.h"
#include "containers/queue.h"

static constexpr i32 BufferSize = 4096;
static constexpr auto GuidComparator = OpComparator<Guid>();
static constexpr auto i32Comparator = OpComparator<i32>();

struct FileRead
{
    i32 offset;
    i32 size;

    inline static i32 Compare(const FileRead& lhs, const FileRead& rhs)
    {
        return lhs.offset - rhs.offset;
    }

    static constexpr Comparable<FileRead> ms_comparable = { Compare };
};

struct StreamFile
{
    Queue<FileRead> reads;
    Array<i32> readerOffsets;
    Array<FStreamDesc> readers;
    IO::FileMap map;
    char path[PIM_PATH];
};

static HashDict<Guid, StreamFile, GuidComparator> ms_files;

namespace FStream
{
    void Request(FStreamDesc desc)
    {
        Guid guid = ToGuid(desc.path);
        StreamFile& file = ms_files[guid];
        if (!file.path[0])
        {
            StrCpy(ARGS(file.path), desc.path);

            EResult err = EUnknown;
            file.map = IO::MapFile(file.path, false);
            ASSERT(IO::IsOpen(file.map));

            file.reads.Init(Alloc_Pool);
            file.readerOffsets.Init(Alloc_Pool);
            file.readers.Init(Alloc_Pool);
        }

        file.readerOffsets.Grow() = desc.offset;
        file.readers.Grow() = desc;
        file.reads.Push({ desc.offset, desc.size }, FileRead::ms_comparable);
    }

    // ------------------------------------------------------------------------

    static void Init()
    {

    }

    static void Update()
    {

    }

    static void Shutdown()
    {

    }

    static constexpr System ms_system =
    {
        ToGuid("FStream"),

    };
};
