cmake_minimum_required(VERSION 3.14)
project(pim_cmake)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_C_STANDARD 11)

#set(CMAKE_BUILD_TYPE Release)

if (WIN32)
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /Ob2 /Oi /GL /GS- /arch:AVX2 /fp:fast /fp:except-")
    set(CMAKE_CXX_FLAGS_DEBUG "/O0")
    set(CMAKE_CXX_FLAGS "/W3 /WX /MP /permissive- /GR-")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/OPT:REF /OPT:ICF /LTCG")
    SET(CMAKE_EXE_LINKER_FLAGS "/WX /DEBUG:FULL /INCREMENTAL:NO")
endif()

if (UNIX)
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
    set(CMAKE_CXX_FLAGS "-Wall -std=c++14 -Wfatal-errors -pthread")
endif()

file(GLOB_RECURSE CPP_FILES "src/*.cpp")
file(GLOB_RECURSE C_FILES "src/*.c")
file(GLOB_RECURSE C_HEADERS "src/*.h")
file(GLOB_RECURSE CPP_HEADERS "src/*.hpp")

add_executable(pim
    ${C_FILES}
    ${CPP_FILES}
    ${C_HEADERS}
    ${CPP_HEADERS}
    "submodules/cJSON/cJSON.c"
    "submodules/cJSON/cJSON_Utils.c"
    "submodules/glfw/src/context.c"
    "submodules/glfw/src/init.c"
    "submodules/glfw/src/input.c"
    "submodules/glfw/src/monitor.c"
    "submodules/glfw/src/osmesa_context.c"
    "submodules/glfw/src/vulkan.c"
    "submodules/glfw/src/window.c"
    "submodules/miniz/miniz.c"
    "submodules/SPIRV-Reflect/spirv_reflect.c"
    "submodules/tlsf/tlsf.c"
    "submodules/volk/volk.c")

if (WIN32)
    target_sources(pim
        PRIVATE
        "submodules/glfw/src/egl_context.c"
        "submodules/glfw/src/wgl_context.c"
        "submodules/glfw/src/win32_init.c"
        "submodules/glfw/src/win32_joystick.c"
        "submodules/glfw/src/win32_monitor.c"
        "submodules/glfw/src/win32_thread.c"
        "submodules/glfw/src/win32_time.c"
        "submodules/glfw/src/win32_window.c"
        "submodules/stackwalker/StackWalker.cpp")
endif()

target_include_directories(pim
    PRIVATE
    "src"
    "submodules"
    "submodules/cgltf"
    "submodules/cJSON"
    "submodules/glfw/include"
    "submodules/miniz"
    "submodules/pcg32"
    "submodules/SPIRV-Reflect"
    "submodules/stackwalker"
    "submodules/stb"
    "submodules/tlsf"
    "submodules/volk"
    "submodules/VulkanMemoryAllocator/src")

if (WIN32)
    target_include_directories(pim
        PRIVATE
        "submodules/embree/embree-3.10.0.x64.vc14.windows/include"
        "submodules/oidn/oidn-1.2.0.x64.vc14.windows/include"
        "$ENV{VK_SDK_PATH}/Include")
endif()

target_compile_definitions(pim
    PRIVATE
    CJSON_HIDE_SYMBOLS
    SOKOL_NO_DEPRECATED)

if (WIN32)
    target_compile_definitions(pim
        PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        _CRT_SECURE_NO_WARNINGS
        VK_USE_PLATFORM_WIN32_KHR
        _GLFW_WIN32)
endif()

target_link_libraries(pim)

if (WIN32)
    set(DLL_FILES
        "submodules/embree/embree-3.10.0.x64.vc14.windows/bin/embree3.dll"
        "submodules/oidn/oidn-1.2.0.x64.vc14.windows/bin/OpenImageDenoise.dll"
        "submodules/oidn/oidn-1.2.0.x64.vc14.windows/bin/tbb.dll"
        "submodules/oidn/oidn-1.2.0.x64.vc14.windows/bin/tbbmalloc.dll")
    file(COPY ${DLL_FILES} DESTINATION ${CMAKE_BINARY_DIR}/Release NO_SOURCE_PERMISSIONS)
    file(COPY ${DLL_FILES} DESTINATION ${CMAKE_BINARY_DIR}/Debug NO_SOURCE_PERMISSIONS)
endif()