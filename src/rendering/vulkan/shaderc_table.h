#pragma once

#include "common/macro.h"
#include "common/library.h"
#include <shaderc/shaderc.h>

PIM_C_BEGIN

typedef struct shaderc_s
{
    library_t lib;

    // Returns a shaderc_compiler_t that can be used to compile modules.
    // A return of NULL indicates that there was an error initializing the compiler.
    // Any function operating on shaderc_compiler_t must offer the basic
    // thread-safety guarantee.
    // [http://herbsutter.com/2014/01/13/gotw-95-solution-thread-safety-and-synchronization/]
    // That is: concurrent invocation of these functions on DIFFERENT objects needs
    // no synchronization; concurrent invocation of these functions on the SAME
    // object requires synchronization IF AND ONLY IF some of them take a non-const
    // argument.
    shaderc_compiler_t(*compiler_initialize)(void);

    // Releases the resources held by the shaderc_compiler_t.
    // After this call it is invalid to make any future calls to functions
    // involving this shaderc_compiler_t.
    void(*compiler_release)(shaderc_compiler_t);

    // Returns a default-initialized shaderc_compile_options_t that can be used
    // to modify the functionality of a compiled module.
    // A return of NULL indicates that there was an error initializing the options.
    // Any function operating on shaderc_compile_options_t must offer the
    // basic thread-safety guarantee.
    shaderc_compile_options_t(*compile_options_initialize)(void);

    // Returns a copy of the given shaderc_compile_options_t.
    // If NULL is passed as the parameter the call is the same as
    // shaderc_compile_options_init.
    shaderc_compile_options_t(*compile_options_clone)(const shaderc_compile_options_t options);

    // Releases the compilation options. It is invalid to use the given
    // shaderc_compile_options_t object in any future calls. It is safe to pass
    // NULL to this function, and doing such will have no effect.
    void(*compile_options_release)(shaderc_compile_options_t options);

    // Adds a predefined macro to the compilation options. This has the same
    // effect as passing -Dname=value to the command-line compiler.  If value
    // is NULL, it has the same effect as passing -Dname to the command-line
    // compiler. If a macro definition with the same name has previously been
    // added, the value is replaced with the new value. The macro name and
    // value are passed in with char pointers, which point to their data, and
    // the lengths of their data. The strings that the name and value pointers
    // point to must remain valid for the duration of the call, but can be
    // modified or deleted after this function has returned. In case of adding
    // a valueless macro, the value argument should be a null pointer or the
    // value_length should be 0u.
    void(*compile_options_add_macro_definition)(
        shaderc_compile_options_t options,
        const char* name,
        size_t name_length,
        const char* value,
        size_t value_length);

    // Sets the source language.  The default is GLSL.
    void(*compile_options_set_source_language)(
        shaderc_compile_options_t options,
        shaderc_source_language lang);

    // Sets the compiler mode to generate debug information in the output.
    void(*compile_options_set_generate_debug_info)(shaderc_compile_options_t options);

    // Sets the compiler optimization level to the given level. Only the last one
    // takes effect if multiple calls of this function exist.
    void(*compile_options_set_optimization_level)(
        shaderc_compile_options_t options,
        shaderc_optimization_level level);

    // Forces the GLSL language version and profile to a given pair. The version
    // number is the same as would appear in the #version annotation in the source.
    // Version and profile specified here overrides the #version annotation in the
    // source. Use profile: 'shaderc_profile_none' for GLSL versions that do not
    // define profiles, e.g. versions below 150.
    void(*compile_options_set_forced_version_profile)(
        shaderc_compile_options_t options,
        int version,
        shaderc_profile profile);

    // Sets includer callback functions.
    void(*compile_options_set_include_callbacks)(
        shaderc_compile_options_t options,
        shaderc_include_resolve_fn resolver,
        shaderc_include_result_release_fn result_releaser,
        void* user_data);

    // Sets the compiler mode to suppress warnings, overriding warnings-as-errors
    // mode. When both suppress-warnings and warnings-as-errors modes are
    // turned on, warning messages will be inhibited, and will not be emitted
    // as error messages.
    void(*compile_options_set_suppress_warnings)(shaderc_compile_options_t options);

    // Sets the target shader environment, affecting which warnings or errors will
    // be issued.  The version will be for distinguishing between different versions
    // of the target environment.  The version value should be either 0 or
    // a value listed in shaderc_env_version.  The 0 value maps to Vulkan 1.0 if
    // |target| is Vulkan, and it maps to OpenGL 4.5 if |target| is OpenGL.
    void(*compile_options_set_target_env)(
        shaderc_compile_options_t options,
        shaderc_target_env target,
        uint32_t version);

    // Sets the target SPIR-V version. The generated module will use this version
    // of SPIR-V.  Each target environment determines what versions of SPIR-V
    // it can consume.  Defaults to the highest version of SPIR-V 1.0 which is
    // required to be supported by the target environment.  E.g. Default to SPIR-V
    // 1.0 for Vulkan 1.0 and SPIR-V 1.3 for Vulkan 1.1.
    void(*compile_options_set_target_spirv)(
        shaderc_compile_options_t options,
        shaderc_spirv_version version);

    // Sets the compiler mode to treat all warnings as errors. Note the
    // suppress-warnings mode overrides this option, i.e. if both
    // warning-as-errors and suppress-warnings modes are set, warnings will not
    // be emitted as error messages.
    void(*compile_options_set_warnings_as_errors)(shaderc_compile_options_t options);

    // Sets a resource limit.
    void(*compile_options_set_limit)(
        shaderc_compile_options_t options,
        shaderc_limit limit,
        int value);

    // Sets whether the compiler should automatically assign bindings to uniforms
    // that aren't already explicitly bound in the shader source.
    void(*compile_options_set_auto_bind_uniforms)(shaderc_compile_options_t options, bool auto_bind);

    // Sets whether the compiler should use HLSL IO mapping rules for bindings.
    // Defaults to false.
    void(*compile_options_set_hlsl_io_mapping)(shaderc_compile_options_t options, bool hlsl_iomap);

    // Sets whether the compiler should determine block member offsets using HLSL
    // packing rules instead of standard GLSL rules.  Defaults to false.  Only
    // affects GLSL compilation.  HLSL rules are always used when compiling HLSL.
    void(*compile_options_set_hlsl_offsets)(shaderc_compile_options_t options, bool hlsl_offsets);

    // Sets the base binding number used for for a uniform resource type when
    // automatically assigning bindings.  For GLSL compilation, sets the lowest
    // automatically assigned number.  For HLSL compilation, the regsiter number
    // assigned to the resource is added to this specified base.
    void(*compile_options_set_binding_base)(
        shaderc_compile_options_t options,
        shaderc_uniform_kind kind,
        uint32_t base);

    // Like shaderc_compile_options_set_binding_base, but only takes effect when
    // compiling a given shader stage.  The stage is assumed to be one of vertex,
    // fragment, tessellation evaluation, tesselation control, geometry, or compute.
    void(*compile_options_set_binding_base_for_stage)(
        shaderc_compile_options_t options,
        shaderc_shader_kind shader_kind,
        shaderc_uniform_kind kind,
        uint32_t base);

    // Sets whether the compiler should automatically assign locations to
    // uniform variables that don't have explicit locations in the shader source.
    void(*compile_options_set_auto_map_locations)(shaderc_compile_options_t options, bool auto_map);

    // Sets a descriptor set and binding for an HLSL register in the given stage.
    // This method keeps a copy of the string data.
    void(*compile_options_set_hlsl_register_set_and_binding_for_stage)(
        shaderc_compile_options_t options,
        shaderc_shader_kind shader_kind,
        const char* reg,
        const char* set,
        const char* binding);

    // Like shaderc_compile_options_set_hlsl_register_set_and_binding_for_stage,
    // but affects all shader stages.
    void(*compile_options_set_hlsl_register_set_and_binding)(
        shaderc_compile_options_t options,
        const char* reg,
        const char* set,
        const char* binding);

    // Sets whether the compiler should enable extension
    // SPV_GOOGLE_hlsl_functionality1.
    void(*compile_options_set_hlsl_functionality1)(shaderc_compile_options_t options, bool enable);

    // Sets whether the compiler should invert position.Y output in vertex shader.
    void(*compile_options_set_invert_y)(shaderc_compile_options_t options, bool enable);

    // Sets whether the compiler generates code for max and min builtins which,
    // if given a NaN operand, will return the other operand. Similarly, the clamp
    // builtin will favour the non-NaN operands, as if clamp were implemented
    // as a composition of max and min.
    void(*compile_options_set_nan_clamp)(shaderc_compile_options_t options, bool enable);

    // Takes a GLSL source string and the associated shader kind, input file
    // name, compiles it according to the given additional_options. If the shader
    // kind is not set to a specified kind, but shaderc_glslc_infer_from_source,
    // the compiler will try to deduce the shader kind from the source
    // string and a failure in deducing will generate an error. Currently only
    // #pragma annotation is supported. If the shader kind is set to one of the
    // default shader kinds, the compiler will fall back to the default shader
    // kind in case it failed to deduce the shader kind from source string.
    // The input_file_name is a null-termintated string. It is used as a tag to
    // identify the source string in cases like emitting error messages. It
    // doesn't have to be a 'file name'.
    // The source string will be compiled into SPIR-V binary and a
    // shaderc_compilation_result will be returned to hold the results.
    // The entry_point_name null-terminated string defines the name of the entry
    // point to associate with this GLSL source. If the additional_options
    // parameter is not null, then the compilation is modified by any options
    // present.  May be safely called from multiple threads without explicit
    // synchronization. If there was failure in allocating the compiler object,
    // null will be returned.
    shaderc_compilation_result_t(*compile_into_spv)(
        const shaderc_compiler_t compiler,
        const char* source_text,
        size_t source_text_size,
        shaderc_shader_kind shader_kind,
        const char* input_file_name,
        const char* entry_point_name,
        const shaderc_compile_options_t additional_options);

    // Like shaderc_compile_into_spv, but the result contains SPIR-V assembly text
    // instead of a SPIR-V binary module.  The SPIR-V assembly syntax is as defined
    // by the SPIRV-Tools open source project.
    shaderc_compilation_result_t(*compile_into_spv_assembly)(
        const shaderc_compiler_t compiler,
        const char* source_text,
        size_t source_text_size,
        shaderc_shader_kind shader_kind,
        const char* input_file_name,
        const char* entry_point_name,
        const shaderc_compile_options_t additional_options);

    // Like shaderc_compile_into_spv, but the result contains preprocessed source
    // code instead of a SPIR-V binary module
    shaderc_compilation_result_t(*compile_into_preprocessed_text)(
        const shaderc_compiler_t compiler,
        const char* source_text,
        size_t source_text_size,
        shaderc_shader_kind shader_kind,
        const char* input_file_name,
        const char* entry_point_name,
        const shaderc_compile_options_t additional_options);

    // Takes an assembly string of the format defined in the SPIRV-Tools project
    // (https://github.com/KhronosGroup/SPIRV-Tools/blob/master/syntax.md),
    // assembles it into SPIR-V binary and a shaderc_compilation_result will be
    // returned to hold the results.
    // The assembling will pick options suitable for assembling specified in the
    // additional_options parameter.
    // May be safely called from multiple threads without explicit synchronization.
    // If there was failure in allocating the compiler object, null will be
    // returned.
    shaderc_compilation_result_t(*assemble_into_spv)(
        const shaderc_compiler_t compiler,
        const char* source_assembly,
        size_t source_assembly_size,
        const shaderc_compile_options_t additional_options);

    // The following functions, operating on shaderc_compilation_result_t objects,
    // offer only the basic thread-safety guarantee.

    // Releases the resources held by the result object. It is invalid to use the
    // result object for any further operations.
    void(*result_release)(shaderc_compilation_result_t result);

    // Returns the number of bytes of the compilation output data in a result
    // object.
    size_t(*result_get_length)(const shaderc_compilation_result_t result);

    // Returns the number of warnings generated during the compilation.
    size_t(*result_get_num_warnings)(const shaderc_compilation_result_t result);

    // Returns the number of errors generated during the compilation.
    size_t(*result_get_num_errors)(const shaderc_compilation_result_t result);

    // Returns the compilation status, indicating whether the compilation succeeded,
    // or failed due to some reasons, like invalid shader stage or compilation
    // errors.
    shaderc_compilation_status(*result_get_compilation_status)(const shaderc_compilation_result_t);

    // Returns a pointer to the start of the compilation output data bytes, either
    // SPIR-V binary or char string. When the source string is compiled into SPIR-V
    // binary, this is guaranteed to be castable to a uint32_t*. If the result
    // contains assembly text or preprocessed source text, the pointer will point to
    // the resulting array of characters.
    const char* (*result_get_bytes)(const shaderc_compilation_result_t result);

    // Returns a null-terminated string that contains any error messages generated
    // during the compilation.
    const char* (*result_get_error_message)(const shaderc_compilation_result_t result);

    // Provides the version & revision of the SPIR-V which will be produced
    void(*get_spv_version)(unsigned int* version, unsigned int* revision);

    // Parses the version and profile from a given null-terminated string
    // containing both version and profile, like: '450core'. Returns false if
    // the string can not be parsed. Returns true when the parsing succeeds. The
    // parsed version and profile are returned through arguments.
    bool(*parse_version_profile)(const char* str, int* version, shaderc_profile* profile);
} shaderc_t;

extern shaderc_t g_shaderc;

bool shaderc_open(void);
void shaderc_close(void);

PIM_C_END
