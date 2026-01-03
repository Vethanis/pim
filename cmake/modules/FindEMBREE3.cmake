
if (MSVC)
    set( _embree3_HEADER_SEARCH_DIRS
        "${CMAKE_SOURCE_DIR}/submodules/embree/embree-3.13.0.x64.vc14.windows/include")

    set( _embree3_LIB_SEARCH_DIRS
        "${CMAKE_SOURCE_DIR}/submodules/embree/embree-3.13.0.x64.vc14.windows/lib")
else()
    set( _embree3_HEADER_SEARCH_DIRS
        "${CMAKE_SOURCE_DIR}/submodules/embree/embree-3.13.5.x86_64.linux/include"
        "/home/linuxbrew/.linuxbrew/include"
        "/usr/local/include"
        "/usr/include")

    set( _embree3_LIB_SEARCH_DIRS
        "${CMAKE_SOURCE_DIR}/submodules/embree/embree-3.13.5.x86_64.linux/lib"
        "/home/linuxbrew/.linuxbrew/lib"
        "/usr/local/lib"
        "/usr/lib")
endif()

find_path(EMBREE3_INCLUDE_DIRS "embree3/rtcore.h"
	PATHS ${_embree3_HEADER_SEARCH_DIRS} )

find_library(EMBREE3_LIBRARIES
	NAMES embree3 embree
	PATHS ${_embree3_LIB_SEARCH_DIRS} )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(EMBREE3 DEFAULT_MSG
	EMBREE3_LIBRARIES EMBREE3_INCLUDE_DIRS)
