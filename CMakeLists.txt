cmake_minimum_required(VERSION 3.14)
project(pim_cmake)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_C_STANDARD 11)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

# -----------------------------------------------------------------------------
# files

file(GLOB_RECURSE PIM_CPP_FILES "src/*.cpp")
file(GLOB_RECURSE PIM_C_FILES "src/*.c")
file(GLOB_RECURSE PIM_C_HEADERS "src/*.h")
file(GLOB_RECURSE PIM_CPP_HEADERS "src/*.hpp")
file(GLOB_RECURSE PIM_HLSL_FILES "src/*.hlsl")

file(GLOB PIM_LUA_FILES "submodules/lua/*.c")
list(FILTER
    PIM_LUA_FILES
    EXCLUDE REGEX
    ".*/lua/(onelua|lua|luac)\.c") # these define main

set(PIM_SUBMODULE_FILES 
    "submodules/cJSON/cJSON.c"
    "submodules/cJSON/cJSON_Utils.c"
    "submodules/imgui/imconfig.h"
    "submodules/imgui/imgui.cpp"
    "submodules/imgui/imgui.h"
    "submodules/imgui/imgui_draw.cpp"
    "submodules/imgui/imgui_internal.h"
    "submodules/imgui/imgui_tables.cpp"
    "submodules/imgui/imgui_widgets.cpp"
    "submodules/imgui/imstb_rectpack.h"
    "submodules/imgui/imstb_textedit.h"
    "submodules/imgui/imstb_truetype.h"
    "submodules/imgui/backends/imgui_impl_glfw.h"
    "submodules/imgui/backends/imgui_impl_glfw.cpp"
    "submodules/cimgui/cimgui.cpp"
    "submodules/cimgui/cimgui.h"
    "submodules/miniz/miniz.c"
    "submodules/miniz/miniz_tdef.c"
    "submodules/miniz/miniz_tinfl.c"
    "submodules/miniz/miniz_zip.c"
    "submodules/SPIRV-Reflect/spirv_reflect.c"
    "submodules/tlsf/tlsf.c"
    "submodules/volk/volk.c"
    "submodules/sokol/sokol_audio.h"
    "submodules/sokol/sokol_time.h"
    ${PIM_LUA_FILES})

set(PIM_INCLUDE_DIRS
    "src"
    "submodules"
    "submodules/imgui"
    "submodules/VulkanMemoryAllocator/src")

set(PIM_MSVC_FILES 
    "submodules/glfw/src/context.c"
    "submodules/glfw/src/init.c"
    "submodules/glfw/src/input.c"
    "submodules/glfw/src/monitor.c"
    "submodules/glfw/src/osmesa_context.c"
    "submodules/glfw/src/vulkan.c"
    "submodules/glfw/src/window.c"
    "submodules/glfw/src/egl_context.c"
    "submodules/glfw/src/wgl_context.c"
    "submodules/glfw/src/win32_init.c"
    "submodules/glfw/src/win32_joystick.c"
    "submodules/glfw/src/win32_monitor.c"
    "submodules/glfw/src/win32_thread.c"
    "submodules/glfw/src/win32_time.c"
    "submodules/glfw/src/win32_window.c"
    "submodules/stackwalker/StackWalker.cpp")

set(PIM_MSVC_INCLUDE_DIRS
    "$ENV{VK_SDK_PATH}/Include"
    "submodules/glfw/include"
    "submodules/embree/embree-3.13.0.x64.vc14.windows/include"
    "submodules/oidn/oidn-1.4.0.x64.vc14.windows/include")

set(PIM_UNIX_INCLUDE_DIRS
    "submodules/embree/embree-3.13.5.x86_64.linux/include"
    "submodules/oidn/oidn-1.4.3.x86_64.linux/include")

set(PIM_MSVC_DLLS
    "submodules/embree/embree-3.13.0.x64.vc14.windows/bin/embree3.dll"
    "submodules/oidn/oidn-1.4.0.x64.vc14.windows/bin/OpenImageDenoise.dll"
    "submodules/oidn/oidn-1.4.0.x64.vc14.windows/bin/tbb12.dll")

set(PIM_UNIX_SOS
    "submodules/embree/embree-3.13.5.x86_64.linux/lib/libembree3.so"
    "submodules/embree/embree-3.13.5.x86_64.linux/lib/libtbb.so"
    "submodules/oidn/oidn-1.4.3.x86_64.linux/lib/libOpenImageDenoise.so")

# -----------------------------------------------------------------------------
# compiler options

set(PIM_COMPILE_DEFINITIONS 
    "IMGUI_DISABLE_DEMO_WINDOWS"
    "CIMGUI_NO_EXPORT"
    "CJSON_HIDE_SYMBOLS"
    "SOKOL_NO_DEPRECATED")

set(PIM_MSVC_COMPILE_DEFINITIONS 
    "NOMINMAX"
    "WIN32_LEAN_AND_MEAN"
    "_CRT_SECURE_NO_WARNINGS"
    "VK_USE_PLATFORM_WIN32_KHR"
    "_GLFW_WIN32")

set(PIM_UNIX_COMPILE_DEFINITIONS
    "VK_USE_PLATFORM_WAYLAND_KHR"
    "LUA_USE_LINUX")

set(PIM_UNIX_COMPILE_DEFINITIONS_DEBUG
    "_DEBUG")

# https://docs.microsoft.com/en-us/cpp/build/reference/compiler-options-listed-alphabetically?view=msvc-150
set(PIM_MSVC_COMPILE_OPTIONS_RELEASE
    "/W3"           # warning level 3
    "/WX"           # warnings are errors
    "/MP"           # multi-process compiles
    "/permissive-"  # make compiler less goofy
    "/GR-"          # disable runtime type information
    "/arch:AVX2"    # vector extensions (modern cpu required)
    "/O2"           # optimization level 2 (the max on msvc)
    "/Ob2"          # inline any suitable function
    "/Oi"           # emit intrinsic functions as their instruction (skip setting TLS errno)
    "/GL"           # link time codegen
    "/fp:fast"      # optimize fp expression order for speed rather than precision and disable fp exceptions
    "/GS-"          # disable security check, it slow
    "/Gw"           # emit global data as comdats to be de-duplicated in ltcg
    "/Gy"           # emit functions as comdats to be de-duplicated in ltcg
    "/volatile:iso" # don't add hardware memory barriers to volatile variable accesses
)

set(PIM_MSVC_COMPILE_OPTIONS_DEBUG
    "/W3"           # warning level 3
    "/WX"           # warnings are errors
    "/MP"           # multi-process compiles
    "/permissive-"  # make compiler less goofy
    "/GR-"          # disable runtime type information
    "/arch:AVX2"    # vector extensions (modern cpu required)
    "/Zi"           # emit debug information in a pdb
)

# https://docs.microsoft.com/en-us/cpp/build/reference/linker-options?view=msvc-150#linker-options-listed-alphabetically
set(PIM_MSVC_LINK_OPTIONS_RELEASE
    "/WX"               # linker warnings are errors
    "/INCREMENTAL:NO"   # disable incremental linking
    "/OPT:REF"          # deduplicate data in ltcg
    "/OPT:ICF"          # deduplicate code in ltcg
    "/LTCG"             # link time code generation
)

set(PIM_MSVC_LINK_OPTIONS_DEBUG
    "/DEBUG:FULL"       # emit debug information in a pdb
    "/WX"               # linker warnings are errors
    "/INCREMENTAL:NO"   # disable incremental linking
)

set(PIM_UNIX_COMPILE_OPTIONS
    "-Wall"
    #"-Werror" # pedantic
    "-Wfatal-errors"
    "-pthread"
    "-ffast-math"
    "-Wno-nullability-completeness"
    "-Wno-unused-function"
    "-Wno-unused-variable"
    "-Wno-unused-local-typedefs" # static assert implementation uses a typedef
    "-Wno-unused-but-set-variable" # currently stb has a warning for this.
    "-Wno-incompatible-pointer-types-discards-qualifiers"
    "-Wno-incompatible-pointer-types")

set(PIM_UNIX_COMPILE_OPTIONS_RELEASE
    "-O3"
)
set(PIM_UNIX_COMPILE_OPTIONS_DEBUG
    "-fexceptions"
    "-O0"
    "-g"
)

# -----------------------------------------------------------------------------
# cmake commands

add_executable(pim
    ${PIM_C_FILES}
    ${PIM_CPP_FILES}
    ${PIM_C_HEADERS}
    ${PIM_CPP_HEADERS}
    ${PIM_SUBMODULE_FILES})

target_include_directories(pim PRIVATE ${PIM_INCLUDE_DIRS})
target_compile_definitions(pim PRIVATE ${PIM_COMPILE_DEFINITIONS})

if (MSVC)
    target_sources(pim PRIVATE ${PIM_MSVC_FILES})
    target_include_directories(pim PRIVATE ${PIM_MSVC_INCLUDE_DIRS})
    target_compile_definitions(pim PRIVATE ${PIM_MSVC_COMPILE_DEFINITIONS})

    target_compile_options(pim PRIVATE "$<$<CONFIG:RELEASE>:${PIM_MSVC_COMPILE_OPTIONS_RELEASE}>")
    target_compile_options(pim PRIVATE "$<$<CONFIG:DEBUG>:${PIM_MSVC_COMPILE_OPTIONS_DEBUG}>")
    target_link_options(pim PRIVATE "$<$<CONFIG:RELEASE>:${PIM_MSVC_LINK_OPTIONS_RELEASE}>")
    target_link_options(pim PRIVATE "$<$<CONFIG:DEBUG>:${PIM_MSVC_LINK_OPTIONS_DEBUG}>")
else()
    target_sources(pim PRIVATE ${PIM_UNIX_FILES})

    find_package(ALSA REQUIRED)
    if (ALSA_FOUND)
        target_include_directories(pim PRIVATE ${ALSA_INCLUDE_DIRS})                  
        target_link_libraries(pim ${ALSA_LIBRARIES}) 
    endif (ALSA_FOUND)

    find_package(GLFW3 REQUIRED)
    if (GLFW3_FOUND)
        target_include_directories(pim PRIVATE ${GLFW3_INCLUDE_DIRS})                  
        target_link_libraries(pim ${GLFW3_LIBRARIES}) 
    endif (GLFW3_FOUND)

    target_include_directories(pim PRIVATE ${PIM_UNIX_INCLUDE_DIRS})
    target_compile_definitions(pim PRIVATE ${PIM_UNIX_COMPILE_DEFINITIONS})
    target_compile_definitions(pim PRIVATE "$<$<CONFIG:DEBUG>:${PIM_UNIX_COMPILE_DEFINITIONS_DEBUG}>")

    target_compile_options(pim PRIVATE ${PIM_UNIX_COMPILE_OPTIONS})
    target_compile_options(pim PRIVATE "$<$<CONFIG:RELEASE>:${PIM_UNIX_COMPILE_OPTIONS_RELEASE}>")
    target_compile_options(pim PRIVATE "$<$<CONFIG:DEBUG>:${PIM_UNIX_COMPILE_OPTIONS_DEBUG}>")
endif()

find_package(EMBREE3 REQUIRED)
if (EMBREE3_FOUND)
    target_include_directories(pim PRIVATE ${EMBREE3_INCLUDE_DIRS})                  
    target_link_libraries(pim ${EMBREE3_LIBRARIES}) 
endif (EMBREE3_FOUND)

find_package(OIDN REQUIRED)
if (OIDN_FOUND)
    target_include_directories(pim PRIVATE ${OIDN_INCLUDE_DIRS})                  
    target_link_libraries(pim ${OIDN_LIBRARIES}) 
endif (OIDN_FOUND)

target_link_libraries(pim)

# -----------------------------------------------------------------------------
# fluff

if (MSVC)
    file(COPY ${PIM_MSVC_DLLS} DESTINATION ${CMAKE_BINARY_DIR}/Release NO_SOURCE_PERMISSIONS)
    file(COPY ${PIM_MSVC_DLLS} DESTINATION ${CMAKE_BINARY_DIR}/Debug NO_SOURCE_PERMISSIONS)
endif()

set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT pim)
set_property(TARGET pim PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

source_group(
    TREE "${CMAKE_SOURCE_DIR}/src"
    FILES
    ${PIM_C_FILES}
    ${PIM_CPP_FILES}
    ${PIM_C_HEADERS}
    ${PIM_CPP_HEADERS}
    ${PIM_HLSL_FILES})

source_group(
    TREE "${CMAKE_SOURCE_DIR}/submodules"
    FILES
    ${PIM_SUBMODULE_FILES}
    ${PIM_MSVC_FILES})
