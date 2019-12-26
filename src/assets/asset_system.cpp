#include "assets/asset_system.h"

#include "containers/array.h"
#include "components/system.h"
#include "quake/packfile.h"

namespace AssetSystem
{
    static Quake::Folder ms_folder;

    static void Init()
    {
        EResult err = EUnknown;
        Array<Quake::DPackFile> arena;
        arena.Init(Alloc_Linear);
        ms_folder = Quake::LoadFolder("packs/id1", arena, err);
        arena.Reset();
    }

    static void Update()
    {

    }

    static void Shutdown()
    {
        Quake::FreeFolder(ms_folder);
    }

    static constexpr Guid ms_dependencies[] =
    {
        ToGuid("InputSystem"),
    };

    static constexpr System ms_system =
    {
        ToGuid("AssetSystem"),
        { ARGS(ms_dependencies) },
        Init,
        Update,
        Shutdown,
    };
    static RegisterSystem ms_register(ms_system);
};
