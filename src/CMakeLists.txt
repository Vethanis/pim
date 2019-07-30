
set(CMAKE_C_STANDARD 11)
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(CMAKE_CXX_FLAGS_DEBUG "/Od /MTd")
set(CMAKE_CXX_FLAGS_RELEASE "/O2 /MT /arch:AVX2 /fp:fast")
set(CMAKE_CXX_FLAGS "/W3 /MP /GR-")

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/submodules)

file(GLOB_RECURSE SOURCES "*.c")
add_executable(${PROJECT_NAME} ${SOURCES})

# target_include_directories(${PROJECT_NAME})

# target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL32)
