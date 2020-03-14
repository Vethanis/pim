#pragma once

#ifdef __cplusplus
    #define PIM_C_BEGIN extern "C" {
    #define PIM_C_END };
#else
    #define PIM_C_BEGIN 
    #define PIM_C_END 
#endif // __cplusplus

// ----------------------------------------------------------------------------

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    #define PLAT_WINDOWS            1
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

#define PLAT_64 (UINTPTR_MAX == UINT64_MAX)
#define PLAT_32 (UINTPTR_MAX == UINT32_MAX)

// ----------------------------------------------------------------------------

#if PLAT_WINDOWS
    #define IF_WIN(x)               x
    #define IF_UNIX(x)              

    #define RESTRICT                __restrict
    #define INTERRUPT()             __debugbreak()
    #define PIM_TLS                 __declspec(thread)

    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
#else
    #define IF_WIN(x)               
    #define IF_UNIX(x)              x

    #define INTERRUPT()             raise(SIGTRAP)
    #define PIM_TLS                 _Thread_local
#endif // PLAT_WINDOWS

#define PIM_EXPORT                  IF_WIN(__declspec(dllexport))
#define PIM_IMPORT                  IF_WIN(__declspec(dllimport))
#define PIM_CDECL                   IF_WIN(__cdecl)

// ----------------------------------------------------------------------------

#ifdef _DEBUG
    #define IF_DEBUG(x)             x
    #define IFN_DEBUG(x)            (void)0
#else
    #define IF_DEBUG(x)             (void)0
    #define IFN_DEBUG(x)            x
#endif // def _DEBUG

// ----------------------------------------------------------------------------

#define NELEM(x)                    ( sizeof(x) / sizeof((x)[0]) )
#define ARGS(x)                     x, NELEM(x)
#define DBG_INT()                   IF_DEBUG(INTERRUPT())
#define IF_TRUE(x, expr)            do { if(x) { expr; } } while(0)
#define IF_FALSE(x, expr)           do { if(!(x)) { expr; } } while(0)
#define ASSERT(x)                   IF_DEBUG(IF_FALSE(x, INTERRUPT()))
#define CHECK(x)                    IF_FALSE(x, DBG_INT(); err = EFail; return retval)
#define CHECKERR()                  IF_FALSE(err == ESuccess, DBG_INT(); return retval)

#define _CAT_TOK(x, y)              x ## y
#define CAT_TOK(x, y)               _CAT_TOK(x, y)

#define SASSERT(x)                  typedef char CAT_TOK(StaticAssert_, __COUNTER__) [ (x) ? 1 : -1]

// ----------------------------------------------------------------------------

// Maximum path length, including null terminator
#define PIM_PATH                    256

// Error codes
#define PIM_ERR_OK                  0
#define PIM_ERR_INTERNAL            1
#define PIM_ERR_ARG_COUNT           2
#define PIM_ERR_ARG_NAME            3
#define PIM_ERR_ARG_NULL            4
#define PIM_ERR_NOT_FOUND           5
