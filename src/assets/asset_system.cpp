#include "assets/asset_system.h"

#include "quake/packfile.h"
#include "containers/hash_dict.h"
#include "ui/imgui.h"
#include "threading/task.h"
#include "common/cvar.h"
#include "common/text.h"
#include "common/sort.h"

static cvar_t cv_imgui;

using File = Quake::DPackFile;
using Header = Quake::DPackHeader;
using Pack = Quake::Pack;
using Folder = Quake::Folder;

static void OnGui();

static HashDict<Text<64>, asset_t> ms_assets;
static Array<Pack> ms_packs;

extern "C" void asset_sys_init(void)
{
    cvar_create(&cv_imgui, "asset_imgui", "1.0");

    ms_assets.Init();
    ms_packs.Init();

    Folder folder = Quake::LoadFolder("packs/id1", EAlloc_Perm);
    for (const Pack& src : folder.packs)
    {
        ms_packs.PushBack(src);
        const Pack& pack = ms_packs.back();
        for (int32_t i = 0; i < pack.count; ++i)
        {
            const File& file = pack.files[i];
            const asset_t asset =
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

extern "C" void asset_sys_update()
{
    if (cv_imgui.asFloat > 0.0f)
    {
        OnGui();
    }
}

extern "C" void asset_sys_shutdown(void)
{
    ms_assets.Reset();
    for (Pack& pack : ms_packs)
    {
        Quake::FreePack(pack);
    }
    ms_packs.Reset();
}

static bool ColumnCmp(int32_t metric, const File* files, int32_t a, int32_t b)
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
static constexpr float ms_uiScales[] =
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

        int32_t used = 0;
        for (int32_t i = 0; i < pack.count; ++i)
        {
            used += pack.files[i].length;
        }
        const int32_t overhead = (sizeof(File) * pack.count) + sizeof(Header);
        const int32_t empty = (pack.bytes - used) - overhead;
        const Header& hdr = *pack.header;

        ImGui::Value("File Count", pack.count);
        ImGui::Bytes("Bytes", pack.bytes);
        ImGui::Bytes("Used", used);
        ImGui::Bytes("Empty", empty);
        ImGui::Value("Header Offset", hdr.offset);
        ImGui::Bytes("Header Length", hdr.length);
        ImGui::Text("Header ID: %c%c%c%c", hdr.id[0], hdr.id[1], hdr.id[2], hdr.id[3]);
        ImGui::Separator();

        Array<int32_t> order = ms_uiTable.Begin(pack.files, pack.count);
        for (int32_t i : order)
        {
            const File& file = pack.files[i];
            ImGui::Text("%d", i); ImGui::NextColumn();
            ImGui::Text("%s", file.name); ImGui::NextColumn();
            ImGui::Text("%d", file.offset); ImGui::NextColumn();
            ImGui::Bytes(file.length); ImGui::NextColumn();
            ImGui::Text("%2.2f%%", (file.length * 100.0f) / (float)used); ImGui::NextColumn();
        }
        ms_uiTable.End(order);

        ImGui::PopID();
    }
    ImGui::End();
}

extern "C" int32_t asset_sys_get(const char* name, asset_t* asset)
{
    ASSERT(name);
    ASSERT(asset);
    return ms_assets.Get(name, *asset);
}
