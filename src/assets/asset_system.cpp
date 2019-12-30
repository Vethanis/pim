#include "assets/asset_system.h"

#include "common/text.h"
#include "common/io.h"
#include "containers/array.h"
#include "containers/queue.h"
#include "containers/hash_dict.h"
#include "components/system.h"
#include "quake/packfile.h"

using PathText = Text<PIM_PATH>;
static constexpr auto GuidComparator = OpComparator<Guid>();

struct Asset
{
    Guid file;
    Guid name;
    Array<Guid> children;
    i32 offset;
    i32 size;
    void* ptr;
    i32 refCount;
};

struct FileOp
{
    u64 time;
    void* ptr;
    i32 offset;
    i32 size;
    bool read;
};

struct File
{
    Guid name;
    IO::FD descriptor;
    Queue<FileOp> ops;
};

static HashDict<Guid, File, GuidComparator> ms_files;
static HashDict<Guid, Asset, GuidComparator> ms_assets;
static Array<Quake::Pack> ms_packs = { Alloc_Pool };

namespace AssetSystem
{
    static void Init()
    {
        Quake::LoadFolder("packs/id1", ms_packs);
        Queue<i32> indices = { Alloc_Linear };
        for (i32 i = 0; i < 1000; ++i)
        {
            indices.Push(1000 - i - 1, OpComparable<i32>());
        }
        for (i32 i = 0; i < 1000; ++i)
        {
            i32 j = indices.Pop();
            ASSERT(j == i);
        }
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
