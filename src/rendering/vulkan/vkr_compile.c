#include "rendering/vulkan/vkr_compile.h"
#include "allocator/allocator.h"
#include "common/stringutil.h"
#include "rendering/vulkan/shaderc_table.h"
#include "io/fstr.h"
#include "io/dir.h"
#include <string.h>

static shaderc_include_result* vkrResolveInclude(
    void* usr,
    const char* includeFile,
    int type,
    const char* srcFile,
    size_t depth)
{
    ASSERT(includeFile);
    ASSERT(srcFile);

    char path[PIM_PATH] = { 0 };
    SPrintf(ARGS(path), "src/shaders/%s", includeFile);

    shaderc_include_result* result = Perm_Calloc(sizeof(*result));

    fstr_t file = fstr_open(path, "rb");
    if (fstr_isopen(file))
    {
        i32 size = (i32)fstr_size(file);
        if (size > 0)
        {
            result->source_name = StrDup(path, EAlloc_Perm);
            result->source_name_length = StrLen(path);
            char* contents = Perm_Alloc(size + 1);
            i32 readLen = fstr_read(file, contents, size);
            contents[size] = 0;
            ASSERT(readLen == size);
            result->content = contents;
            result->content_length = size;
        }
        fstr_close(&file);
    }

    return result;
}

static void vkrReleaseInclude(void* usr, shaderc_include_result* result)
{
    if (result)
    {
        Mem_Free((void*)result->source_name);
        Mem_Free((void*)result->content);
        Mem_Free(result);
    }
}

static shaderc_shader_kind vkrShaderTypeToShaderKind(vkrShaderType type)
{
    switch (type)
    {
    default:
        ASSERT(false);
        return shaderc_glsl_infer_from_source;

    case vkrShaderType_Vert:
        return shaderc_vertex_shader;
    case vkrShaderType_Frag:
        return shaderc_fragment_shader;
    case vkrShaderType_Comp:
        return shaderc_compute_shader;

    case vkrShaderType_AnyHit:
        return shaderc_anyhit_shader;
    case vkrShaderType_Call:
        return shaderc_callable_shader;
    case vkrShaderType_ClosestHit:
        return shaderc_closesthit_shader;
    case vkrShaderType_Isect:
        return shaderc_intersection_shader;
    case vkrShaderType_Miss:
        return shaderc_miss_shader;
    case vkrShaderType_Raygen:
        return shaderc_raygen_shader;
    case vkrShaderType_Task:
        return shaderc_task_shader;
    case vkrShaderType_Mesh:
        return shaderc_mesh_shader;
    }
}

bool vkrCompile(const vkrCompileInput* input, vkrCompileOutput* output)
{
    ASSERT(input);
    ASSERT(output);

    memset(output, 0, sizeof(*output));

    if (!shaderc_open())
    {
        ASSERT(false);
        return false;
    }

    if (!input->text)
    {
        ASSERT(false);
        return false;
    }
    if (!input->filename)
    {
        ASSERT(false);
        return false;
    }
    if (!input->entrypoint)
    {
        ASSERT(false);
        return false;
    }

    shaderc_compiler_t compiler = g_shaderc.compiler_initialize();
    ASSERT(compiler);
    if (!compiler)
    {
        return false;
    }
    shaderc_compile_options_t options = g_shaderc.compile_options_initialize();
    ASSERT(options);
    if (!options)
    {
        g_shaderc.compiler_release(compiler);
        return false;
    }

    g_shaderc.compile_options_set_source_language(
        options, shaderc_source_language_hlsl);
    g_shaderc.compile_options_set_optimization_level(
        options, shaderc_optimization_level_performance);
    g_shaderc.compile_options_set_target_env(
        options, shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
    // shaderc: internal error: compilation succeeded but failed to optimize: 2nd operand of Decorate: operand BufferBlock(3) requires SPIR-V version 1.3 or earlier
    // shaderc breaks itself by using BufferBlock on SPIRV 1.4 :(
    g_shaderc.compile_options_set_target_spirv(options, shaderc_spirv_version_1_3);
    g_shaderc.compile_options_set_warnings_as_errors(options);
    g_shaderc.compile_options_set_auto_bind_uniforms(options, true);
    g_shaderc.compile_options_set_auto_map_locations(options, true);
    g_shaderc.compile_options_set_hlsl_io_mapping(options, true);
    g_shaderc.compile_options_set_hlsl_offsets(options, true);
    //g_shaderc.compile_options_set_hlsl_functionality1(options, true);
    g_shaderc.compile_options_set_nan_clamp(options, false);

    {
        const i32 macroCount = input->macroCount;
        const char** macroKeys = input->macroKeys;
        const char** macroValues = input->macroValues;
        for (i32 i = 0; i < macroCount; ++i)
        {
            const char* key = macroKeys[i];
            const char* value = macroValues[i];
            if (key && key[0])
            {
                i32 keyLen = StrLen(key);
                i32 valueLen = StrLen(value);
                g_shaderc.compile_options_add_macro_definition(
                    options, key, keyLen, value, valueLen);
            }
        }
    }

    g_shaderc.compile_options_set_include_callbacks(
        options, vkrResolveInclude, vkrReleaseInclude, NULL);

    const i32 inputLen = StrLen(input->text);
    const shaderc_shader_kind kind = vkrShaderTypeToShaderKind(input->type);

    shaderc_compilation_status status = 0;
    if (input->compile)
    {
        shaderc_compilation_result_t result =
            g_shaderc.compile_into_spv(
                compiler,
                input->text,
                inputLen,
                kind,
                input->filename,
                input->entrypoint,
                options);
        ASSERT(result);
        if (result)
        {
            status = g_shaderc.result_get_compilation_status(result);

            const char* errors = g_shaderc.result_get_error_message(result);
            output->errors = StrDup(errors, EAlloc_Perm);

            const i32 numBytes = (i32)g_shaderc.result_get_length(result);
            const void* spirv = g_shaderc.result_get_bytes(result);
            ASSERT(numBytes >= 0);
            if ((numBytes > 0) && spirv)
            {
                output->dwordCount = numBytes / sizeof(u32);
                output->dwords = Perm_Alloc(numBytes);
                memcpy(output->dwords, spirv, numBytes);
            }

            g_shaderc.result_release(result);
        }
    }

    if (input->disassemble)
    {
        shaderc_compilation_result_t result =
            g_shaderc.compile_into_spv_assembly(
                compiler,
                input->text,
                inputLen,
                kind,
                input->filename,
                input->entrypoint,
                options);
        ASSERT(result);
        if (result)
        {
            status = g_shaderc.result_get_compilation_status(result);
            const char* disassembly = g_shaderc.result_get_bytes(result);
            output->disassembly = StrDup(disassembly, EAlloc_Perm);
            g_shaderc.result_release(result);
        }
    }

    g_shaderc.compile_options_release(options);
    g_shaderc.compiler_release(compiler);

    return status == shaderc_compilation_status_success;
}

void vkrCompileOutput_Del(vkrCompileOutput* output)
{
    if (output)
    {
        Mem_Free(output->disassembly);
        Mem_Free(output->dwords);
        Mem_Free(output->errors);
        memset(output, 0, sizeof(*output));
    }
}

char* vkrLoadShader(const char* filename)
{
    char* contents = NULL;

    char path[PIM_PATH] = { 0 };
    SPrintf(ARGS(path), "src/shaders/%s", filename);
    StrPath(ARGS(path));

    fstr_t fd = fstr_open(path, "rb");
    if (fstr_isopen(fd))
    {
        i32 size = (i32)fstr_size(fd);
        if (size > 0)
        {
            contents = Perm_Alloc(size + 1);
            i32 readLen = fstr_read(fd, contents, size);
            contents[size] = 0;
            ASSERT(readLen == size);
        }
        fstr_close(&fd);
    }
    else
    {
        ASSERT(false);
    }

    return contents;
}
