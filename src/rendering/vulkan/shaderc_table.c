#include "rendering/vulkan/shaderc_table.h"
#include <string.h>

#define LOAD_FN(sym) \
    g_shaderc.sym = Library_Sym(g_shaderc.lib, STR_TOK(shaderc_##sym)); \
    ASSERT(g_shaderc.sym)

shaderc_t g_shaderc;

bool shaderc_open(void)
{
    if (g_shaderc.lib.handle)
    {
        return true;
    }

    Library lib = Library_Open("shaderc_shared");
    if (!lib.handle)
    {
        return false;
    }

    g_shaderc.lib = lib;

    LOAD_FN(compiler_initialize);
    LOAD_FN(compiler_release);
    LOAD_FN(compile_options_initialize);
    LOAD_FN(compile_options_clone);
    LOAD_FN(compile_options_release);
    LOAD_FN(compile_options_add_macro_definition);
    LOAD_FN(compile_options_set_source_language);
    LOAD_FN(compile_options_set_generate_debug_info);
    LOAD_FN(compile_options_set_optimization_level);
    LOAD_FN(compile_options_set_forced_version_profile);
    LOAD_FN(compile_options_set_include_callbacks);
    LOAD_FN(compile_options_set_suppress_warnings);
    LOAD_FN(compile_options_set_target_env);
    LOAD_FN(compile_options_set_target_spirv);
    LOAD_FN(compile_options_set_warnings_as_errors);
    LOAD_FN(compile_options_set_limit);
    LOAD_FN(compile_options_set_auto_bind_uniforms);
    LOAD_FN(compile_options_set_hlsl_io_mapping);
    LOAD_FN(compile_options_set_hlsl_offsets);
    LOAD_FN(compile_options_set_binding_base);
    LOAD_FN(compile_options_set_binding_base_for_stage);
    LOAD_FN(compile_options_set_auto_map_locations);
    LOAD_FN(compile_options_set_hlsl_register_set_and_binding_for_stage);
    LOAD_FN(compile_options_set_hlsl_register_set_and_binding);
    LOAD_FN(compile_options_set_hlsl_functionality1);
    LOAD_FN(compile_options_set_invert_y);
    LOAD_FN(compile_options_set_nan_clamp);
    LOAD_FN(compile_into_spv);
    LOAD_FN(compile_into_spv_assembly);
    LOAD_FN(compile_into_preprocessed_text);
    LOAD_FN(assemble_into_spv);
    LOAD_FN(result_release);
    LOAD_FN(result_get_length);
    LOAD_FN(result_get_num_warnings);
    LOAD_FN(result_get_num_errors);
    LOAD_FN(result_get_compilation_status);
    LOAD_FN(result_get_bytes);
    LOAD_FN(result_get_error_message);
    LOAD_FN(get_spv_version);
    LOAD_FN(parse_version_profile);

    return true;
}

void shaderc_close(void)
{
    if (g_shaderc.lib.handle)
    {
        Library_Close(g_shaderc.lib);
        memset(&g_shaderc, 0, sizeof(g_shaderc));
    }
}
