#pragma once

#include "common/macro.h"

#if PLAT_WINDOWS
    #define SOKOL_D3D11             1
#elif PLAT_LINUX
    #define SOKOL_GLCORE33          1
#elif PLAT_MAC
    #define SOKOL_METAL             1
#elif PLAT_ANDROID
    #define SOKOL_GLES3             1
#elif PLAT_IOS
    #define SOKOL_METAL             1
#elif PLAT_IOS_SIM
    #define SOKOL_METAL             1
#endif // CUR_PLAT == PLAT_WINDOWS

#define SOKOL_ASSERT                Assert
#define SOKOL_WIN32_FORCE_MAIN      1
