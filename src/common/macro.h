#pragma once

#ifdef __cplusplus
    #define PIM_C_BEGIN extern "C" {
    #define PIM_C_END };
#else
    #define PIM_C_BEGIN 
    #define PIM_C_END 
#endif // __cplusplus

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

#if PLAT_WINDOWS
    #define IF_WIN(x)               x
    #define IF_UNIX(x)              

    #define INTERRUPT()             __debugbreak()
    #define pim_thread_local        __declspec(thread)
    #define PIM_EXPORT              __declspec(dllexport)
    #define PIM_IMPORT              __declspec(dllimport)
    #define PIM_CDECL               __cdecl
    #define VEC_CALL                __vectorcall
    #define pim_inline              __forceinline
    #define pim_noalias             __restrict
    #define pim_alignas(x)          __declspec(align(x))
    #define pim_optimize            __pragma(optimize("gt", on))
    #define pim_deoptimize          __pragma(optimize("gt", off))

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
    #define pim_thread_local        _Thread_local
    #define PIM_EXPORT              
    #define PIM_IMPORT              
    #define PIM_CDECL               
    #define VEC_CALL                
    #define pim_inline              static
    #define pim_noalias             
    #define pim_alignas(x)          
    #define pim_optimize            
    #define pim_deoptimize          
#endif // PLAT_WINDOWS

#define NELEM(x)                    ( sizeof(x) / sizeof((x)[0]) )
#define ARGS(x)                     x, NELEM(x)
#define IF_TRUE(x, expr)            do { if(x) { expr; } } while(0)
#define IF_FALSE(x, expr)           do { if(!(x)) { expr; } } while(0)

#ifdef _DEBUG
    #define IF_DEBUG(x)             x
    #define IFN_DEBUG(x)            (void)0
    #define ASSERT(x)               IF_FALSE(x, INTERRUPT())
    #define CONFIG_STR              "Debug"
#else
    #define IF_DEBUG(x)             (void)0
    #define IFN_DEBUG(x)            x
    #define ASSERT(x)               (void)0
    #define CONFIG_STR              "Release"
#endif // def _DEBUG

#define _CAT_TOK(x, y)              x ## y
#define CAT_TOK(x, y)               _CAT_TOK(x, y)
#define STR_TOK(x)                  #x

#define SASSERT(x)                  typedef char CAT_TOK(StaticAssert_, __COUNTER__) [ (x) ? 1 : -1]

#define PIM_PATH                    256
#define PIM_FWD_DECL(name)          typedef struct name name
#define PIM_DECL_HANDLE(name)       typedef struct name##_T* name

PIM_C_BEGIN

typedef signed char                 i8;
typedef signed short                i16;
typedef signed int                  i32;
typedef signed long long            i64;
typedef i64                         isize;
typedef unsigned char               u8;
typedef unsigned short              u16;
typedef unsigned int                u32;
typedef unsigned long long          u64;
typedef u64                         usize;

#ifndef NULL
    #ifdef __cplusplus
        #define NULL                0
    #else
        #define NULL                ((void*)0)
    #endif // cpp
#endif

#ifndef _STDBOOL
#define _STDBOOL
#define __bool_true_false_are_defined 1
#ifndef __cplusplus
    #define bool                    _Bool
    #define false                   0
    #define true                    1
#endif // __cplusplus
#endif // _STDBOOL

typedef enum
{
    EAlloc_Perm = 0,
    EAlloc_Temp,
    EAlloc_Task,
    EAlloc_Count
} EAlloc;

#define kMaxThreads 256

PIM_C_END
