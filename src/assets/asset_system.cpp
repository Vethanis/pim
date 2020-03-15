#include "assets/asset_system.h"

#include "components/system.h"
#include "quake/packfile.h"
#include "containers/hash_dict.h"
#include "ui/imgui.h"
#include "threading/task.h"
#include "common/io.h"
#include "common/cvar.h"
#include "common/text.h"
#include "common/sort.h"

static cvar_t cv_imgui;

namespace AssetSystem
{
    using File = Quake::DPackFile;
    using Header = Quake::DPackHeader;
    using Pack = Quake::Pack;
    using Folder = Quake::Folder;

    static void Init();
    static void Update();
    static void Shutdown();
    static void OnGui();
    static void OnFolder(Folder& folder);

    struct LoadTask final : ITask
    {
        Text<64> m_path;
        Folder m_folder;
        LoadTask() : ITask(0, 1, 1){}
        void Execute(i32, i32) final { m_folder = Quake::LoadFolder(m_path, Alloc_Perm); }
    };

    static HashDict<Text<64>, Asset> ms_assets;
    static Array<Pack> ms_packs;
    static LoadTask ms_task;

    static System ms_system{ "AssetSystem", { "RenderSystem", "TaskSystem" }, Init, Update, Shutdown, };

    static void Init()
    {
        cvar_create(&cv_imgui, "asset_imgui", "1.0");

        ms_assets.Init();
        ms_packs.Init();

        ms_task.m_path = "packs/id1";
        TaskSystem::Submit(&ms_task);
    }

    static void Update()
    {
        if (ms_task.IsComplete())
        {
            OnFolder(ms_task.m_folder);
            ms_task = LoadTask();
        }

        if (cv_imgui.asFloat > 0.0f)
        {
            OnGui();
        }
    }

    static void Shutdown()
    {
        ms_assets.Reset();
        for (Pack& pack : ms_packs)
        {
            Quake::FreePack(pack);
        }
        ms_packs.Reset();
    }

    static void OnFolder(Folder& folder)
    {
        for (const Pack& src : folder.packs)
        {
            ms_packs.PushBack(src);
            const Pack& pack = ms_packs.back();
            for (i32 i = 0; i < pack.count; ++i)
            {
                const File& file = pack.files[i];
                const Asset asset =
                {
                    file.name,
                    pack.path,
                    file.offset,
                    file.length,
                    pack.ptr + file.offset,
                };

                if (!ms_assets.Add(file.name, asset))
                {
                    ms_assets.Set(file.name, asset);
                }
            }
        }
        folder.packs.Reset();
    }

    static bool ColumnCmp(i32 metric, const File* files, i32 a, i32 b)
    {
        switch (metric)
        {
        default:
        case 0: return a < b;
        case 1: return strcmp(files[a].name, files[b].name) < 0;
        case 2: return files[a].offset < files[b].offset;
        case 3: return files[a].length < files[b].length;
        case 4: return files[a].length < files[b].length;
        }
    }

    static constexpr const char* ms_uiTitles[] =
    {
         "Index", "Name", "Offset", "Size", "% Usage",
    };
    static constexpr f32 ms_uiScales[] =
    {
        0.1f, 0.4f, 0.15f, 0.15f, 0.15f,
    };
    static ImGui::Table<File> ms_uiTable =
    {
        ms_uiTitles,
        ms_uiScales,
        NELEM(ms_uiTitles),
        ColumnCmp,
    };

    static void OnGui()
    {
        ImGui::SetNextWindowScale(0.7f, 0.7f);
        ImGui::Begin("AssetSystem");
        for (const Pack& pack : ms_packs)
        {
            if (!ImGui::CollapsingHeader(pack.path))
            {
                continue;
            }

            ImGui::PushID(pack.path);

            i32 used = 0;
            for (i32 i = 0; i < pack.count; ++i)
            {
                used += pack.files[i].length;
            }
            const i32 overhead = (sizeof(File) * pack.count) + sizeof(Header);
            const i32 empty = (pack.bytes - used) - overhead;
            const Header& hdr = *pack.header;

            ImGui::Value("File Count", pack.count);
            ImGui::Bytes("Bytes", pack.bytes);
            ImGui::Bytes("Used", used);
            ImGui::Bytes("Empty", empty);
            ImGui::Value("Header Offset", hdr.offset);
            ImGui::Bytes("Header Length", hdr.length);
            ImGui::Text("Header ID: %c%c%c%c", hdr.id[0], hdr.id[1], hdr.id[2], hdr.id[3]);
            ImGui::Separator();

            Array<i32> order = ms_uiTable.Begin(pack.files, pack.count);
            for (i32 i : order)
            {
                const File& file = pack.files[i];
                ImGui::Text("%d", i); ImGui::NextColumn();
                ImGui::Text("%s", file.name); ImGui::NextColumn();
                ImGui::Text("%d", file.offset); ImGui::NextColumn();
                ImGui::Bytes(file.length); ImGui::NextColumn();
                ImGui::Text("%2.2f%%", (file.length * 100.0f) / (f32)used); ImGui::NextColumn();
            }
            ms_uiTable.End(order);

            ImGui::PopID();
        }
        ImGui::End();
    }

    bool Get(cstr name, Asset& asset)
    {
        ASSERT(name);
        return ms_assets.Get(name, asset);
    }
};
