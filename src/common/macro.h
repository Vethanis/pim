#pragma once

// ----------------------------------------------------------------------------

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    #define PLAT_WINDOWS            1
    #ifdef _WIN64
        #define PLAT_64             1
    #endif
#elif defined(__ANDROID__)
    #define PLAT_ANDROID            1
#elif defined(__APPLE__)
    #if defined(TARGET_IPHONE_SIMULATOR)
        #define PLAT_IOS_SIM        1
    #elif defined(TARGET_OS_IPHONE)
        #define PLAT_IOS            1
    #elif defined(TARGET_OS_MAC)
        #define PLAT_MAC            1
    #endif // def TARGET_IPHONE_SIMULATOR
#elif defined(__linux__)
    #define PLAT_LINUX              1
#else
    #error Unable to detect current platform
#endif // def _WIN32 || def __CYGWIN__

// ----------------------------------------------------------------------------

#if PLAT_WINDOWS
    #define RESTRICT                __restrict
    #define INTERRUPT()             __debugbreak()
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif

    #define IF_WIN32(x)             x
    #define IF_UNIX(x)              
#else
    #define IF_WIN32(x)             
    #define IF_UNIX(x)              x
#endif // PLAT_WINDOWS

// ----------------------------------------------------------------------------

#ifdef _DEBUG
    #define IF_DEBUG(x)             x
    #define IFN_DEBUG(x)            
#else
    #define IF_DEBUG(x)             
    #define IFN_DEBUG(x)            x
#endif // def _DEBUG

// ----------------------------------------------------------------------------

#define NELEM(x)                    ( sizeof(x) / sizeof((x)[0]) )
#define ARGS(x)                     x, NELEM(x)
#define DBG_INT()                   IF_DEBUG(INTERRUPT())
#define IF_TRUE(x, expr)            if(x) { expr; }
#define IF_FALSE(x, expr)           if(!(x)) { expr; }
#define ASSERT(x)                   IF_DEBUG(IF_FALSE(x, INTERRUPT()))
#define CHECK(x)                    IF_FALSE(x, DBG_INT(); err = EFail; return retval)
#define CHECKERR()                  IF_FALSE(err == ESuccess, DBG_INT(); return retval)

#define _CAT_TOK(x, y)              x ## y
#define CAT_TOK(x, y)               _CAT_TOK(x, y)

#define SASSERT(x)                  typedef char CAT_TOK(StaticAssert_, __COUNTER__) [ (x) ? 1 : -1]

// Maximum path length, including null terminator
#define PIM_PATH                    260
