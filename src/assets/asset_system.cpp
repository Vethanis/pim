#include "assets/asset_system.h"

#include "common/io.h"
#include "common/text.h"
#include "containers/array.h"
#include "containers/queue.h"
#include "containers/hash_dict.h"
#include "containers/hash_set.h"
#include "components/system.h"
#include "quake/packfile.h"

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
