if (MSVC)
    set( _oidn_HEADER_SEARCH_DIRS
        "${CMAKE_SOURCE_DIR}/submodules/oidn/oidn-1.4.0.x64.vc14.windows/include")

    set( _oidn_LIB_SEARCH_DIRS
        "${CMAKE_SOURCE_DIR}/submodules/oidn/oidn-1.4.0.x64.vc14.windows/lib")
else()
    set( _oidn_HEADER_SEARCH_DIRS
        "${CMAKE_SOURCE_DIR}/submodules/oidn/oidn-1.4.3.x86_64.linux/include"
        "/home/linuxbrew/.linuxbrew/include"
        "/usr/local/include"
        "/usr/include")

    set( _oidn_LIB_SEARCH_DIRS
        "${CMAKE_SOURCE_DIR}/submodules/oidn/oidn-1.4.3.x86_64.linux/lib"
        "/home/linuxbrew/.linuxbrew/lib"
        "/usr/local/lib"
        "/usr/lib")
endif()

find_path(OIDN_INCLUDE_DIRS "OpenImageDenoise/oidn.h"
	PATHS ${_oidn_HEADER_SEARCH_DIRS} )

find_library(OIDN_LIBRARIES
	NAMES OpenImageDenoise
	PATHS ${_oidn_LIB_SEARCH_DIRS} )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OIDN DEFAULT_MSG
	OIDN_LIBRARIES OIDN_INCLUDE_DIRS)
