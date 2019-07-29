#pragma once

#define PLAT_UNKNOWN                    0
#define PLAT_WINDOWS                    1
#define PLAT_LINUX                      2
#define PLAT_MAC                        3
#define PLAT_ANDROID                    4
#define PLAT_IOS                        5
#define PLAT_IOS_SIM                    6

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    #define CUR_PLAT                    PLAT_WINDOWS
#elif defined(__ANDROID__)
    #define CUR_PLAT                    PLAT_ANDROID
#elif defined(__APPLE__)
    #if defined(TARGET_IPHONE_SIMULATOR)
        #define CUR_PLAT                PLAT_IOS_SIM
    #elif defined(TARGET_OS_IPHONE)
        #define CUR_PLAT                PLAT_IOS
    #elif defined(TARGET_OS_MAC)
        #define CUR_PLAT                PLAT_MAC
    #endif // TARGET_IPHONE_SIMULATOR
#elif defined(__linux__)
    #define CUR_PLAT                    PLAT_LINUX
#else
    #define CUR_PLAT                    PLAT_UNKNOWN
#endif // _WIN32 || __CYGWIN__
