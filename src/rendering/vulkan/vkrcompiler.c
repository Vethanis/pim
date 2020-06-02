//#include "rendering/vulkan/vkrcompiler.h"
//#include <shaderc/shaderc.h>
//#include <string.h>
//#include "allocator/allocator.h"
//#include "common/stringutil.h"
//
//static shaderc_include_result* vkrResolveInclude(
//    void* usr,
//    const char* includeFile,
//    int type,
//    const char* srcFile,
//    size_t depth)
//{
//    bool relative = type == shaderc_include_type_relative; // #include "foo"
//    bool standard = type == shaderc_include_type_standard; // #include <foo>
//    ASSERT(includeFile);
//    ASSERT(srcFile);
//
//    shaderc_include_result* result = perm_calloc(sizeof(*result));
//
//    return result;
//}
//
//static void vkrReleaseInclude(void* usr, shaderc_include_result* result)
//{
//    if (result)
//    {
//        pim_free((void*)result->content);
//        pim_free((void*)result->source_name);
//        pim_free(result);
//    }
//}
//
//bool vkrCompile(const vkrCompileInput* input, vkrCompileOutput* output)
//{
//    ASSERT(input);
//    ASSERT(output);
//    bool status = false;
//
//    shaderc_compiler_t compiler = shaderc_compiler_initialize();
//    ASSERT(compiler);
//    shaderc_compile_options_t options = shaderc_compile_options_initialize();
//    ASSERT(options);
//
//    shaderc_compile_options_set_source_language(
//        options, shaderc_source_language_hlsl);
//    shaderc_compile_options_set_optimization_level(
//        options, shaderc_optimization_level_performance);
//
//    for (i32 i = 0; i < input->macroCount; ++i)
//    {
//        ASSERT(input->macroKeys);
//        ASSERT(input->macroValues);
//        const char* key = input->macroKeys[i];
//        const char* value = input->macroValues[i];
//        i32 keyLen = StrLen(key);
//        i32 valueLen = StrLen(value);
//        if (key)
//        {
//            shaderc_compile_options_add_macro_definition(
//                options, key, keyLen, value, valueLen);
//        }
//    }
//
//    shaderc_compile_options_set_include_callbacks(
//        options, vkrResolveInclude, vkrReleaseInclude, NULL);
//
//    if (options)
//    {
//        shaderc_compile_options_release(options);
//    }
//    if (compiler)
//    {
//        shaderc_compiler_release(compiler);
//    }
//    return status;
//}
