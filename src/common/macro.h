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
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
#else
    #define IF_WIN(x)               
    #define IF_UNIX(x)              x
#endif // PLAT_WINDOWS

#ifdef _MSC_VER
    #define INTERRUPT()             __debugbreak()
    #define pim_thread_local        __declspec(thread)
    #define PIM_EXPORT              __declspec(dllexport)
    #define PIM_IMPORT              __declspec(dllimport)
    #define PIM_CDECL               __cdecl
    #define VEC_CALL                __vectorcall
    #define pim_inline              __forceinline
    #define pim_noalias             __restrict
    #define pim_alignas(x)          __declspec(align(x))
    #define pim_optimize            __pragma(optimize("", on))
    #define pim_deoptimize          __pragma(optimize("", off))
#else
    #define INTERRUPT()             raise(SIGTRAP)
    #define pim_thread_local        _Thread_local
    #define PIM_EXPORT              
    #define PIM_IMPORT              
    #define PIM_CDECL               
    #define VEC_CALL                __vectorcall
    #define pim_inline              __attribute__((always_inline))
    #define pim_noalias             __restrict__
    #define pim_alignas(x)          _Alignas(x)
    #define pim_optimize            _Pragma("clang optimize on")
    #define pim_deoptimize          _Pragma("clang optimize off")
#endif // PLAT_WINDOWS

#define NELEM(x)                    ( sizeof(x) / sizeof((x)[0]) )
#define ARGS(x)                     x, NELEM(x)
#define IF_TRUE(x, expr)            do { if(x) { expr; } } while(0)
#define IF_FALSE(x, expr)           do { if(!(x)) { expr; } } while(0)
#define pim_offsetof(s, m)          ((usize)&(((s*)0)->m))
#define pim_alignof(x)              _Alignof(x)
#define pim_min(a, b)               ((a) < (b) ? (a) : (b))
#define pim_max(a, b)               ((a) > (b) ? (a) : (b))

#define REL_ASSERT(x)               IF_FALSE(x, INTERRUPT())

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
#define _STR_TOK(x)                  #x
#define STR_TOK(x)                  _STR_TOK(x)

#define SASSERT(x)                  typedef char CAT_TOK(StaticAssert_, __COUNTER__) [ (x) ? 1 : -1]

#define PIM_FILELINE                CAT_TOK(__FILE__, STR_TOK(__LINE__))
#define PIM_PATH                    256
#define PIM_FWD_DECL(name)          typedef struct name name
#define PIM_DECL_HANDLE(name)       typedef struct name##_T* name

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

SASSERT(sizeof(i8) == 1);
SASSERT(sizeof(u8) == 1);
SASSERT(sizeof(i16) == 2);
SASSERT(sizeof(u16) == 2);
SASSERT(sizeof(i32) == 4);
SASSERT(sizeof(u32) == 4);
SASSERT(sizeof(i64) == 8);
SASSERT(sizeof(u64) == 8);

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
    EAlloc_Texture,
    EAlloc_Temp,

    EAlloc_COUNT
} EAlloc;

#define kMaxThreads                 256

#define VK_ENABLE_BETA_EXTENSIONS   1
#define VKR_KHRONOS_LAYER_NAME      "VK_LAYER_KHRONOS_validation"
#define VKR_ASSIST_LAYER_NAME       "VK_LAYER_LUNARG_assistant_layer"

#if defined(_DEBUG) && 1
    // these are very slow!
    #define VKR_KHRONOS_LAYER_ON    1
    #define VKR_ASSIST_LAYER_ON     1
#else
    #define VKR_KHRONOS_LAYER_ON    0
    #define VKR_ASSIST_LAYER_ON     0
#endif // _DEBUG

#define VKR_DEBUG_MESSENGER_ON      (VKR_KHRONOS_LAYER_ON || VKR_ASSIST_LAYER_ON)
