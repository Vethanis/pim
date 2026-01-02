set( _glfw3_HEADER_SEARCH_DIRS
	"${CMAKE_SOURCE_DIR}/submodules/include"
	"/home/linuxbrew/.linuxbrew/include"
	"/usr/local/include"
	"/usr/include")

set( _glfw3_LIB_SEARCH_DIRS
	"${CMAKE_SOURCE_DIR}/submodules/lib"
	"/home/linuxbrew/.linuxbrew/lib"
	"/usr/local/lib"
	"/usr/lib")

find_path(GLFW3_INCLUDE_DIRS "GLFW/glfw3.h"
	PATHS ${_glfw3_HEADER_SEARCH_DIRS} )

find_library(GLFW3_LIBRARIES
	NAMES glfw3 glfw
	PATHS ${_glfw3_LIB_SEARCH_DIRS} )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLFW3 DEFAULT_MSG
	GLFW3_LIBRARIES GLFW3_INCLUDE_DIRS)
