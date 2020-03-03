#include "assets/stream_file.h"
#include "containers/hash_dict.h"
#include "common/guid.h"
#include "common/text.h"
#include "components/system.h"
#include "os/thread.h"
#include "common/io.h"
#include "ui/imgui.h"
#include "common/cvar.h"

static CVar cv_imgui("StreamFile::ImGui", "0.0");

namespace StreamFile
{
    static OS::Mutex ms_lock;
    static HashDict<PathText, IO::FileMap> ms_files;

    struct StreamFileSystem final : ISystem
    {
        StreamFileSystem() : ISystem("StreamFile", { "RenderSystem" }) {}
        void Init() final
        {
            ms_lock.Open();
            ms_files.Init();
        }
        void Update() final
        {
            if (cv_imgui.AsBool())
            {
                ImGui::Begin("StreamFile");
                {
                    OS::LockGuard guard(ms_lock);
                    for (auto pair : ms_files)
                    {
                        ImGui::Separator();
                        ImGui::Text("Path: %s", pair.key.begin());
                        ImGui::Text("Size: %d", pair.value.memory.size());
                        ImGui::Text("Descriptor: %d", pair.value.fd.fd);
                        ImGui::Text("Mapping: %p", pair.value.hMapping);
                    }
                }
                ImGui::End();
            }
        }
        void Shutdown() final
        {
            ms_lock.Lock();
            for (auto pair : ms_files)
            {
                IO::Close(pair.value);
            }
            ms_files.Reset();
            ms_lock.Close();
        }
    };

    static StreamFileSystem ms_system;

    static bool GetFile(cstr path, IO::FileMap& file)
    {
        ASSERT(path);
        const PathText text(path);
        OS::LockGuard guard(ms_lock);
        return ms_files.Get(text, file);
    }

    static bool AddFile(cstr path, const IO::FileMap& file)
    {
        ASSERT(path);
        ASSERT(IO::IsOpen(file));
        const PathText text(path);
        OS::LockGuard guard(ms_lock);
        return ms_files.Add(text, file);
    }

    static bool GetAddFile(cstr path, IO::FileMap& file)
    {
        if (!GetFile(path, file))
        {
            EResult err = EUnknown;
            IO::FD fd = IO::Open(path, IO::OBinRandRW, err);
            if (err != ESuccess)
            {
                return false;
            }

            file = IO::MapFile(fd, true, err);
            if (err != ESuccess)
            {
                IO::Close(fd, err);
                return false;
            }

            if (!AddFile(path, file))
            {
                IO::Close(file);
                return GetFile(path, file);
            }
        }
        return true;
    }

    EResult Read(cstr path, Slice<u8>& dst, i32 offset, i32 length)
    {
        ASSERT(path);
        ASSERT(offset >= 0);

        Slice<u8> src = {};

        IO::FileMap file = {};
        if (GetAddFile(path, file))
        {
            if (length > 0)
            {
                if (file.memory.ValidSlice(offset, length))
                {
                    src = file.memory.Subslice(offset, length);
                }
            }
            else if (length == -1)
            {
                // -1 => grab entire file
                src = file.memory;
            }

            if (src.size() > 0)
            {
                void* pDst = Allocator::Alloc(Alloc_Perm, src.size());
                ASSERT(pDst);
                memcpy(pDst, src.begin(), src.size());
                dst = { (u8*)pDst, src.size() };
                return ESuccess;
            }
        }

        return EFail;
    }

    EResult Write(cstr path, Slice<const u8> src, i32 offset)
    {
        ASSERT(path);
        ASSERT(offset >= 0);
        const i32 length = src.size();

        IO::FileMap file = {};
        if (GetAddFile(path, file))
        {
            if (length > 0)
            {
                if (file.memory.ValidSlice(offset, length))
                {
                    Slice<u8> dst = file.memory.Subslice(offset, length);
                    memcpy(dst.begin(), src.begin(), length);
                    return ESuccess;
                }
            }
        }

        return EFail;
    }
};
