#include "assets/asset_system.h"

#include "containers/array.h"
#include "quake/packfile.h"

namespace AssetSystem
{
    static Quake::Folder ms_folder;

    void Init()
    {
        EResult err = EUnknown;
        Array<Quake::DPackFile> arena;
        arena.Init(Alloc_Linear);
        ms_folder = Quake::LoadFolder("packs/id1", arena, err);
        arena.Reset();
    }

    void Update()
    {

    }

    void Shutdown()
    {
        Quake::FreeFolder(ms_folder);
    }
}
