#include "assets/asset_system.h"

#include "common/text.h"
#include "containers/array.h"
#include "containers/hash_set.h"
#include "containers/queue.h"
#include "components/system.h"
#include "quake/packfile.h"

using PathText = Text<PIM_PATH>;

struct Children
{
    Array<Guid> Values;
};

struct AssetInMem
{
    void* ptr;
    i32 size;
    i32 refCount;
};

struct AssetInFile
{
    Guid filename;
    i32 offset;
    i32 size;
};

struct FileOp
{
    u64 time;
    bool read;
    void* ptr;
    i32 size;
    i32 offset;
};

struct FileQueue
{
    Queue<FileOp> Value;
};

struct StreamFiles
{
    Array<PathText> paths;
    Array<Guid> names;
    Array<IO::FD> descriptors;
    Array<FileQueue> queues;
};

static constexpr auto GuidComparator = OpComparator<Guid>();
static Array<Guid> ms_names = { Alloc_Pool };
static Array<Children> ms_children = { Alloc_Pool };
static StreamFiles ms_files;

static Array<Quake::Pack> ms_packs = { Alloc_Pool };

namespace AssetSystem
{
    static void Init()
    {
        Quake::LoadFolder("packs/id1", ms_packs);
        Queue<i32> indices = { Alloc_Linear };
        indices.Push(5);
        indices.Push(4);
        indices.Push(99);
        indices.Push(1);
        indices.Sort(OpComparator<i32>());
        ASSERT(indices.Pop() == 1);
        ASSERT(indices.Pop() == 4);
        ASSERT(indices.Pop() == 5);
        ASSERT(indices.Pop() == 99);
    }

    static void Update()
    {

    }

    static void Shutdown()
    {
        Quake::FreeFolder(ms_packs);
    }

    static constexpr System ms_system =
    {
        ToGuid("AssetSystem"),
        { 0, 0 },
        Init,
        Update,
        Shutdown,
    };
    static RegisterSystem ms_register(ms_system);
};
