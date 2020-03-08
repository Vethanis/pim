#include "assets/asset_system.h"

#include "assets/assetdb.h"
#include "components/system.h"
#include "quake/packfile.h"
#include "containers/hash_dict.h"
#include "ui/imgui.h"
#include "common/io.h"
#include "common/cvar.h"
#include "common/guid.h"

static CVar cv_imgui("assets_ui", "1.0");

namespace AssetSystem
{
    static HashDict<Guid, Asset> ms_assets;
    static Quake::Folder ms_folder;
    static Array<Quake::Pack> ms_packs;

    static void Init();
    static void Update();
    static void Shutdown();
    static void OnGui();

    static void Init()
    {
        ms_assets.Init();
        ms_packs.Init();

        ms_folder = Quake::LoadFolder("packs/id1", Alloc_Perm);

        for (const Quake::Pack& pack : ms_packs)
        {
            for (i32 i = 0; i < pack.count; ++i)
            {
                const Quake::DPackFile& file = pack.files[i];
                const Guid guid = ToGuid(file.name);
                const Asset asset =
                {
                    file.name,
                    pack.path,
                    file.offset,
                    file.length,
                    NULL,
                    0
                };
                if (!ms_assets.Add(guid, asset))
                {
                    ms_assets.Set(guid, asset);
                }
            }
        }
    }

    static void Update()
    {
        if (cv_imgui.AsBool())
        {
            OnGui();
        }
    }

    static void Shutdown()
    {
        ms_assets.Reset();
        Quake::FreeFolder(ms_folder);
    }

    static void OnGui()
    {
        ImGui::Begin("AssetSystem");
        {
            for (const Quake::Pack& pack : ms_folder.packs)
            {
                if (ImGui::CollapsingHeader(pack.path))
                {
                    i32 used = 0;
                    for (i32 i = 0; i < pack.count; ++i)
                    {
                        used += pack.files[i].length;
                    }
                    const i32 overhead = (sizeof(Quake::DPackFile) * pack.count) + sizeof(Quake::DPackHeader);
                    const i32 empty = (pack.bytes - used) - overhead;

                    ImGui::PushID(pack.path);
                    ImGui::Value("File Count", pack.count);
                    ImGui::Bytes("Bytes", pack.bytes);
                    ImGui::Bytes("Used", used);
                    ImGui::Bytes("Empty", empty);
                    ImGui::Value("Header Offset", pack.header->offset);
                    ImGui::Bytes("Header Length", pack.header->length);
                    ImGui::Text("Header ID: %c%c%c%c", pack.header->id[0], pack.header->id[1], pack.header->id[2], pack.header->id[3]);
                    ImGui::Separator();

                    for (i32 i = 0; i < pack.count; ++i)
                    {
                        const Quake::DPackFile& file = pack.files[i];
                        ImGui::Text("Name: %s", file.name);
                        ImGui::Text("Usage: %2.2f%%", (file.length * 100.0f) / (f32)used);
                        ImGui::Value("Offset", file.offset);
                        ImGui::Bytes("Length", file.length);
                        ImGui::Separator();
                    }
                    ImGui::PopID();
                }
            }
        }
        ImGui::End();
    }

    //bool Exists(cstr name)
    //{
    //    ASSERT(name);
    //}

    //bool IsLoaded(cstr name)
    //{
    //    ASSERT(name);
    //}

    //bool Load(cstr name)
    //{

    //}

    //Slice<u8> Acquire(cstr name)
    //{
    //    ASSERT(name);
    //}

    //void Release(cstr name)
    //{
    //    ASSERT(name);
    //}

    static System ms_system{ "AssetSystem", {"RenderSystem"}, Init, Update, Shutdown, };
};
