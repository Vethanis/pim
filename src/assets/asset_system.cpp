#include "assets/asset_system.h"

#include "assets/packdb.h"
#include "assets/assetdb.h"
#include "assets/file_streaming.h"

#include "components/system.h"
#include "quake/packfile.h"

static PackTable ms_packs;
static AssetTable ms_assets;

static i32 Compare(const Quake::DPackFile& lhs, const Quake::DPackFile& rhs)
{
    return lhs.offset - rhs.offset;
}

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
            Pack* pPack = ms_packs.Get(packId);
            ASSERT(!pPack);
            if (!pPack)
            {
                pPack = ms_packs.New();
                pPack->Init(pack.path, pack.size);
                ms_packs.Add(packId, pPack);
            }

            pPack->Remove({ 0, sizeof(Quake::DPackHeader) });
            pPack->Remove({ pack.header.offset, pack.header.length });
            Sort(pack.files.begin(), pack.files.size(), { Compare });

            for (const Quake::DPackFile& file : pack.files)
            {
                pPack->Remove({ file.offset, file.length });

                const Guid assetId = ToGuid(file.name);
                Asset* pAsset = ms_assets.Get(assetId);
                if (!pAsset)
                {
                    pAsset = ms_assets.New();
                    pAsset->Init(file.name, packId, { file.offset, file.length });
                    bool added = ms_assets.Add(assetId, pAsset);
                    ASSERT(added);
                }
            }
        }
        Quake::FreeFolder(folder);
    }

    static void Update()
    {

    }

    static void Shutdown()
    {
        ms_packs.Shutdown();
        ms_assets.Shutdown();
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
