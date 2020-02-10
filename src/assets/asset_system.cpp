#include "assets/asset_system.h"

#include "assets/packdb.h"
#include "assets/assetdb.h"
#include "assets/file_streaming.h"
#include "components/system.h"
#include "quake/packfile.h"

static PackTable ms_packs;
static AssetTable ms_assets;

namespace AssetSystem
{
    static void Init()
    {
        ms_packs.Init();
        ms_assets.Init();

        Quake::Folder folder = Quake::LoadFolder("packs/id1", Alloc_Stack);
        for (Quake::Pack& pack : folder.packs)
        {
            const Guid packId = ToGuid(pack.path);
            for (const Quake::DPackFile& file : pack.files)
            {
                const Guid assetId = ToGuid(file.name);
                ms_assets.GetAdd(assetId, { file.name, packId, { file.offset, file.length } });
            }
        }
        Quake::FreeFolder(folder);
    }

    static void Update()
    {

    }

    static void Shutdown()
    {
        ms_packs.Reset();
        ms_assets.Reset();
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
