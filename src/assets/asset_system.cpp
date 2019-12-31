#include "assets/asset_system.h"

#include "common/text.h"
#include "common/io.h"
#include "containers/array.h"
#include "containers/queue.h"
#include "containers/hash_dict.h"
#include "containers/hash_set.h"
#include "components/system.h"
#include "quake/packfile.h"

static constexpr auto GuidComparator = OpComparator<Guid>();

using PathText = Text<PIM_PATH>;
using GuidSet = HashSet<Guid, GuidComparator>;
using GuidToIndex = HashDict<Guid, i32, GuidComparator>;
using GuidToGuid = HashDict<Guid, Guid, GuidComparator>;

struct FileLocation
{
    i32 offset;
    i32 size;
};

struct FilePack
{
    Array<Guid> assets;
    Array<FileLocation> locations;
};

namespace Assets
{
    static GuidToIndex lookup;
    static Array<Guid> names;
    static Array<GuidSet> children;
    static Array<Guid> files;
    static Array<i32> refcounts;
    static Array<Slice<u8>> memory;
};

namespace Files
{
    static GuidToIndex lookup;
    static Array<Guid> names;
    static Array<PathText> paths;
    static Array<GuidToIndex> packLookups;
    static Array<FilePack> packs;
};

namespace AssetSystem
{
    static void Init()
    {
        Quake::Folder folder = Quake::LoadFolder("packs/id1", Alloc_Stack);
        Quake::FreeFolder(folder);
    }

    static void Update()
    {

    }

    static void Shutdown()
    {

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
