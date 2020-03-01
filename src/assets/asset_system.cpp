#include "assets/asset_system.h"

#include "assets/assetdb.h"
#include "components/system.h"
#include "quake/packfile.h"
#include "assets/stream_file.h"
#include "containers/mtqueue.h"
#include "ui/imgui.h"
#include "common/io.h"
#include "os/atomics.h"

namespace AssetSystem
{
    struct LoadTask final : ReadTask
    {
        LoadTask() : ReadTask(), m_pAsset(0) {}
        void Setup(Asset* pAsset)
        {
            ASSERT(pAsset);
            m_pAsset = pAsset;
            ReadTask::Setup(pAsset->pack, pAsset->offset, pAsset->size);
        }
        void OnResult(EResult err, Slice<u8> data) final;
    private:
        Asset* m_pAsset;
    };

    static AssetTable ms_assets;
    static ObjPool<LoadTask> ms_taskPool;

    struct System final : ISystem
    {
        System() : ISystem("AssetSystem", { "RenderSystem" }) {}

        void Init() final
        {
            ms_assets.Init();
            ms_taskPool.Init();

            Quake::Folder folder = Quake::LoadFolder("packs/id1", Alloc_Temp);
            for (const Quake::Pack& pack : folder.packs)
            {
                for (const Quake::DPackFile& file : pack.files)
                {
                    ms_assets.GetAdd(
                        file.name,
                        {
                            file.name,
                            pack.path,
                            file.offset,
                            file.length
                        });
                }
            }
            Quake::FreeFolder(folder);
        }

        void Update() final
        {
            ImGui::Begin("AssetSystem");
            {
                OS::LockGuard guard(ms_assets.m_mutex);
                for (auto pair : ms_assets.m_dict)
                {
                    const Asset* pAsset = pair.value;
                    ASSERT(pAsset);
                    ImGui::Separator();
                    ImGui::Text("Name: %s", pAsset->name.begin());
                    ImGui::Text("Pack: %s", pAsset->pack.begin());
                    ImGui::Text("Location: (%d, %d)", pAsset->offset, pAsset->size);
                    ImGui::Text("RefCount: %d", pAsset->refCount);
                    ImGui::Text("Loaded: %s", LoadPtr(pAsset->pData) ? "true" : "false");
                }
            }
            ImGui::End();
        }

        void Shutdown() final
        {
            ms_taskPool.Reset();
            ms_assets.m_mutex.Lock();
            for (auto pair : ms_assets.m_dict)
            {
                Allocator::Free(pair.value->pData);
            }
            ms_assets.Reset();
        }
    };
    static System ms_system;

    void LoadTask::OnResult(EResult err, Slice<u8> data)
    {
        Asset* pAsset = m_pAsset;
        ASSERT(pAsset);
        if (err == ESuccess)
        {
            void* pSrc = data.begin();
            ASSERT(pSrc);
            ASSERT(data.size() == pAsset->size);

            bool wrote = false;
            void* pDst = LoadPtr(pAsset->pData);
            if (!pDst)
            {
                wrote = CmpExStrongPtr(pAsset->pData, pDst, pSrc);
            }

            if (!wrote)
            {
                ASSERT(pDst);
                Allocator::Free(pSrc);
            }
        }
        else
        {
            IO::Printf(IO::StdErr, "Failed to load file %s\n", pAsset->name);
        }

        ms_taskPool.Delete(this);
    }

    bool Exists(cstr name)
    {
        ASSERT(name);
        return ms_assets.Get(name) != 0;
    }

    bool IsLoaded(cstr name)
    {
        ASSERT(name);

        const void* ptr = 0;
        const Asset* pAsset = ms_assets.Get(name);
        if (pAsset)
        {
            ptr = LoadPtr(pAsset->pData);
        }

        return ptr != 0;
    }

    ITask* CreateLoad(cstr name)
    {
        ASSERT(name);
        Asset* pAsset = ms_assets.Get(name);
        if (pAsset)
        {
            const i32 rc = Inc(pAsset->refCount);
            ASSERT(rc >= 0);

            if (LoadPtr(pAsset->pData))
            {
                return 0;
            }

            LoadTask* pTask = ms_taskPool.New();
            ASSERT(pTask);
            pTask->Setup(pAsset);
            return pTask;
        }
        return 0;
    }

    void FreeLoad(ITask* pLoadTask)
    {
        if (pLoadTask)
        {
            ms_taskPool.Delete(static_cast<LoadTask*>(pLoadTask));
        }
    }

    Slice<u8> Acquire(cstr name)
    {
        ASSERT(name);

        Asset* pAsset = ms_assets.Get(name);
        if (pAsset)
        {
            const i32 rc = Inc(pAsset->refCount);
            ASSERT(rc >= 0);
            return { (u8*)LoadPtr(pAsset->pData), pAsset->size };
        }
        return { 0, 0 };
    }

    void Release(cstr name)
    {
        ASSERT(name);

        Asset* pAsset = ms_assets.Get(name);
        if (pAsset)
        {
            i32 prev = Load(pAsset->refCount);
            u64 spins = 0;
            while (
                (prev > 0) &&
                !CmpExStrong(pAsset->refCount, prev, prev - 1))
            {
                OS::Spin(++spins);
            }

            ASSERT(prev >= 0);
            if (prev == 1)
            {
                void* pData = ExchangePtr(pAsset->pData, (void*)0);
                Allocator::Free(pData);
            }
        }
    }
};
