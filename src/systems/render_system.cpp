#include "systems/render_system.h"

#include <sokol/sokol_gfx.h>
#include <sokol/sokol_app.h>
#include <sokol/util/sokol_imgui.h>
#include <stdio.h>

#include "common/int_types.h"
#include "common/stringutil.h"
#include "containers/array.h"
#include "systems/time_system.h"
#include "common/io.h"
#include "common/hashstring.h"
#include "common/text.h"

namespace ShaderSystem
{
    enum ShaderType
    {
        ShaderType_Pixel = 0,
        ShaderType_Vertex,
        ShaderType_Geometry,
        ShaderType_Hull,
        ShaderType_Domain,
        ShaderType_Compute,

        ShaderType_Count
    };

    static cstrc ShaderTypeTags[] =
    {
        "_ps.hlsl",
        "_vs.hlsl",
        "_gs.hlsl",
        "_hs.hlsl",
        "_ds.hlsl",
        "_cs.hlsl",
    };

    static cstrc ProfileStrs[] =
    {
        "ps_6_3",
        "vs_6_3",
        "gs_6_3",
        "hs_6_3",
        "ds_6_3",
        "cs_6_3",
    };

    using Bytecode = Slice<u8>;

    struct Shaders
    {
        Array<HashString> names;
        Array<ShaderType> types;
        Array<Bytecode> bytecode;
    };

    DeclareNS(Shaders);

    static Shaders ms_shaders;

    static ShaderType GetShaderType(cstr shaderName)
    {
        for (i32 i = 0; i < CountOf(ShaderTypeTags); ++i)
        {
            if (StrIStr(shaderName, ShaderTypeTags[i]))
            {
                return (ShaderType)i;
            }
        }
        return ShaderType_Count;
    }

    static void Init()
    {
        char cmd[1024];
        char cwd[PIM_MAX_PATH];
        char tmp[PIM_MAX_PATH];

        cmd[0] = 0;
        cwd[0] = 0;
        tmp[0] = 0;

        cwd[0] = 0;
        IO::GetCwd(cwd, CountOf(cwd));

        tmp[0] = 0;
        SPrintf(tmp, "%s/build", cwd);
        FixPath(tmp);
        IO::MkDir(tmp);

        tmp[0] = 0;
        SPrintf(tmp, "%s/build/shaders", cwd);
        FixPath(tmp);
        IO::MkDir(tmp);

        IO::Finder finder = {};
        IO::FindData findData = {};
        while (IO::Find(finder, findData, "src/shaders/*.hlsl"))
        {
            const ShaderType type = GetShaderType(findData.name);
            if (type == ShaderType_Count)
            {
                continue;
            }

            ms_shaders.names.grow() = HashString(NS_Shaders, findData.name);
            ms_shaders.types.grow() = type;

            cmd[0] = 0;
            SPrintf(cmd, " %s/tools/dxc/bin/dxc.exe -WX", cwd);
            StrCatf(cmd, " -Fo %s/build/shaders/%s", cwd, findData.name);
            StrIRep(cmd, ".hlsl", ".cso");
            StrCatf(cmd, " %s/src/shaders/%s", cwd, findData.name);
            StrCatf(cmd, " -I %s/src", cwd);
            StrCatf(cmd, " -T %s", ProfileStrs[type]);
            FixPath(cmd);

            puts(cmd);

            IO::Stream dxcStdout = IO::POpen(cmd, "rb");
            if (IO::IsOpen(dxcStdout))
            {
                while (IO::FRead(dxcStdout, tmp, CountOf(tmp)))
                {
                    printf("%s", tmp);
                }
                IO::PClose(dxcStdout);
            }

            tmp[0] = 0;
            SPrintf(tmp, "%s/build/shaders/%s", cwd, findData.name);
            StrIRep(tmp, ".hlsl", ".cso");
            FixPath(tmp);

            Slice<u8> dxil = ms_shaders.bytecode.grow();

            IO::Stream cso = IO::FOpen(tmp, "rb");
            if (IO::IsOpen(cso))
            {
                const usize dxilSize = IO::FSize(cso);
                dxil = Allocator::Alloc<u8>(dxilSize);
                isize got = IO::FRead(cso, dxil.begin(), dxilSize);
                IO::FClose(cso);
                DebugAssert(got == dxilSize);
            }
        }
    }
    static void Shutdown()
    {
        for (Bytecode& bc : ms_shaders.bytecode)
        {
            Allocator::Free(bc);
        }
        ms_shaders.bytecode.reset();
        ms_shaders.names.reset();
        ms_shaders.types.reset();
    }
};

namespace RenderSystem
{
    constexpr sg_pass_action ms_clear =
    {
        0,
        {
            {
                SG_ACTION_CLEAR,
                { 0.25f, 0.25f, 0.5f, 0.0f }
            },
        },
    };

    static i32 ms_width;
    static i32 ms_height;

    void Init()
    {
        {
            sg_desc desc = {};
            desc.mtl_device = sapp_metal_get_device();
            desc.mtl_drawable_cb = sapp_metal_get_drawable;
            desc.mtl_renderpass_descriptor_cb = sapp_metal_get_renderpass_descriptor;
            desc.d3d11_device = sapp_d3d11_get_device();
            desc.d3d11_device_context = sapp_d3d11_get_device_context();
            desc.d3d11_render_target_view_cb = sapp_d3d11_get_render_target_view;
            desc.d3d11_depth_stencil_view_cb = sapp_d3d11_get_depth_stencil_view;
            sg_setup(&desc);
        }
        {
            simgui_desc_t desc = {};
            simgui_setup(&desc);
        }
        ms_width = sapp_width();
        ms_height = sapp_height();

        ShaderSystem::Init();
    }

    void Update()
    {
        ms_width = sapp_width();
        ms_height = sapp_height();
        simgui_new_frame(ms_width, ms_height, TimeSystem::DeltaTimeF32());
    }

    void Shutdown()
    {
        simgui_shutdown();
        sg_shutdown();

        ShaderSystem::Shutdown();
    }

    void FrameEnd()
    {
        sg_begin_default_pass(&ms_clear, ms_width, ms_height);
        {
            simgui_render();
        }
        sg_end_pass();

        sg_commit();
    }

    bool OnEvent(const sapp_event* evt)
    {
        return simgui_handle_event(evt);
    }

    void Visualize()
    {

    }

    System GetSystem()
    {
        System sys;
        sys.Init = Init;
        sys.Update = Update;
        sys.Shutdown = Shutdown;
        sys.Visualize = Visualize;
        sys.enabled = true;
        sys.visualizing = false;
        return sys;
    }
};