#include "rendering/shader_system.h"

#include "common/macro.h"
#include "common/hashstring.h"
#include "common/io.h"
#include "containers/array.h"


namespace ShaderSystem
{
    DeclareHashNS(Shaders);

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

    struct Shaders
    {
        Array<HashString> names;
        Array<ShaderType> types;
        Array<Allocation> bytecode;
    };
    static Shaders ms_shaders;
    static IO::Module ms_dxc;

    static ShaderType GetShaderType(cstrc shaderName, i32 size)
    {
        for (i32 i = 0; i < countof(ShaderTypeTags); ++i)
        {
            if (StrIStr(shaderName, size, ShaderTypeTags[i]))
            {
                return (ShaderType)i;
            }
        }
        return ShaderType_Count;
    }

    void Init()
    {
        char cmd[1024] = {};
        char cwd[PIM_PATH] = {};
        char tmp[PIM_PATH] = {};

        IO::GetCwd(argof(cwd));

        SPrintf(argof(tmp), "%s/build", cwd);
        FixPath(argof(tmp));
        IO::MkDir(tmp);

        SPrintf(argof(tmp), "%s/build/shaders", cwd);
        FixPath(argof(tmp));
        IO::MkDir(tmp);

        Array<IO::FindData> findDatas = {};
        Array<IO::Stream> procs = {};
        IO::FindAll(findDatas, "src/shaders/*.hlsl");

        Array<char> googleHtml = {};
        IO::Curl("https://google.com", googleHtml);
        IO::Puts("\n\nResult:\n\n");
        IO::Write(IO::StdOut, googleHtml.begin(), googleHtml.sizeBytes());
        IO::Puts("\n\n");
        googleHtml.reset();

        for (i32 i = 0; i < findDatas.size(); ++i)
        {
            const ShaderType type = GetShaderType(argof(findDatas[i].name));
            if (type == ShaderType_Count)
            {
                findDatas.remove(i--);
                continue;
            }
            cstrc hlslName = findDatas[i].name;

            ms_shaders.names.grow() = HashString(NS_Shaders, hlslName);
            ms_shaders.types.grow() = type;

            SPrintf(argof(cmd), " %s/tools/dxc/bin/dxc.exe -WX", cwd);
            StrCatf(argof(cmd), " -Fo %s/build/shaders/%s", cwd, hlslName);
            StrIRep(argof(cmd), ".hlsl", ".cso");
            StrCatf(argof(cmd), " %s/src/shaders/%s", cwd, hlslName);
            StrCatf(argof(cmd), " -I %s/src", cwd);
            StrCatf(argof(cmd), " -T %s", ProfileStrs[type]);
            FixPath(argof(cmd));

            procs.grow() = IO::POpen(cmd, "rb");
        }

        for (i32 i = 0; i < procs.size(); ++i)
        {
            SPrintf(argof(tmp), "%s/build/shaders/%s", cwd, findDatas[i].name);
            StrIRep(argof(tmp), ".hlsl", ".cso");
            FixPath(argof(tmp));

            Allocation dxil = {};

            IO::PClose(procs[i]);
            IO::FD cso = IO::Open(tmp, IO::OBinSeqRead);
            if (IO::IsOpen(cso))
            {
                const i32 dxilSize = (i32)IO::Size(cso);
                dxil = Allocator::_Alloc(dxilSize);
                const i32 got = IO::Read(cso, dxil.begin(), dxilSize);
                DebugAssert(got == dxilSize);
                IO::Close(cso);
            }

            ms_shaders.bytecode.grow() = dxil;
        }

        findDatas.reset();
        procs.reset();
    }

    void Update()
    {

    }

    void Shutdown()
    {
        for (Allocation& bc : ms_shaders.bytecode)
        {
            Allocator::_Free(bc);
        }
        ms_shaders.bytecode.reset();
        ms_shaders.names.reset();
        ms_shaders.types.reset();
    }

    void Visualize()
    {

    }
};
