#include "assets/asset_system.h"

#include "assets/packdb.h"
#include "assets/assetdb.h"
#include "assets/file_streaming.h"
#include "components/system.h"
#include "quake/packfile.h"

namespace AssetSystem
{
    static PackTable ms_packs;
    static AssetTable ms_assets;

    struct System final : ISystem
    {
        System() : ISystem("AssetSystem") {}

        void Init() final
        {
            ms_packs.Init();
            ms_assets.Init();

            Quake::Folder folder = Quake::LoadFolder("packs/id1", Alloc_Temp);
            for (Quake::Pack& pack : folder.packs)
            {
                const Guid packId = ToGuid(pack.path);
                for (const Quake::DPackFile& file : pack.files)
                {
                    const Guid assetId = ToGuid(file.name);
                    //ms_assets.GetAdd(assetId, { file.name, packId, { file.offset, file.length } });
                }
            }
            Quake::FreeFolder(folder);
        }
        void Update() final
        {

        }
        void Shutdown() final
        {
            //ms_packs.Reset();
            //ms_assets.Reset();
        }
    };
    static System ms_system;
};
