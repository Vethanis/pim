
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_DEBUG "/Od /MTd /Zi")
set(CMAKE_CXX_FLAGS_RELEASE "/O2 /MT /arch:AVX2 /fp:fast")
set(CMAKE_CXX_FLAGS "/W3 /MP /GR-")

include_directories(${CMAKE_SOURCE_DIR}/submodules)

file(GLOB_RECURSE SOURCES "*.cpp" "*.c" "*.cc")
add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME}
    PUBLIC glfw)

target_link_libraries(${PROJECT_NAME}
    PRIVATE glfw
    PRIVATE OpenGL32)
