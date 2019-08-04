#pragma once

#include "common/platform.h"

#if CUR_PLAT == PLAT_WINDOWS
    #define SOKOL_GLCORE33          1
#elif CUR_PLAT == PLAT_LINUX
    #define SOKOL_GLCORE33          1
#elif CUR_PLAT == PLAT_MAC
    #define SOKOL_METAL             1
#elif CUR_PLAT == PLAT_ANDROID
    #define SOKOL_GLES3             1
#elif CUR_PLAT == PLAT_IOS
    #define SOKOL_METAL             1
#elif CUR_PLAT == PLAT_IOS_SIM
    #define SOKOL_METAL             1
#else
    #error Unhandled platform
#endif // CUR_PLAT == PLAT_WINDOWS
