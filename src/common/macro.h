#pragma once

#ifdef __cplusplus
#   define PIM_C_BEGIN extern "C" {
#   define PIM_C_END };
#else
#   define PIM_C_BEGIN 
#   define PIM_C_END 
#endif // __cplusplus

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
#   define PLAT_WINDOWS            1
#elif defined(__ANDROID__)
#   define PLAT_ANDROID            1
#elif defined(__APPLE__)
#   if defined(TARGET_IPHONE_SIMULATOR)
#       define PLAT_IOS_SIM        1
#   elif defined(TARGET_OS_IPHONE)
#       define PLAT_IOS            1
#   elif defined(TARGET_OS_MAC)
#       define PLAT_MAC            1
#   endif // def TARGET_IPHONE_SIMULATOR
#elif defined(__linux__)
#   define PLAT_LINUX              1
#else
#   error Unable to detect current platform
#endif // def _WIN32 || def __CYGWIN__

#if PLAT_WINDOWS
#   define IF_WIN(x)                x
#   define IF_UNIX(x)               
#   ifndef WIN32_LEAN_AND_MEAN
#       define WIN32_LEAN_AND_MEAN
#       endif
#   ifndef NOMINMAX
#       define NOMINMAX
#   endif
#else
#   define IF_WIN(x)                
#   define IF_UNIX(x)               x
#endif // PLAT_WINDOWS

#if defined(__clang__)
#   define COMPILER_CLANG           1
#   define COMPILER_GCC             0
#   define COMPILER_MSVC            0
#elif defined(__GNUC__) || defined(__GNUG__)
#   define COMPILER_CLANG           0
#   define COMPILER_GCC             1
#   define COMPILER_MSVC            0
#elif defined(_MSC_VER)
#   define COMPILER_CLANG           0
#   define COMPILER_GCC             0
#   define COMPILER_MSVC            1
#else
#   error Unrecognized C compiler
#endif // defined(__clang__)

#if COMPILER_MSVC
#   define INTERRUPT()              __debugbreak()
#   define SASSERT(x)               typedef char CAT_TOK(StaticAssert_, __COUNTER__) [ (x) ? 1 : -1]
#   define pim_thread_local         __declspec(thread)
#   define PIM_EXPORT               __declspec(dllexport)
#   define PIM_IMPORT               __declspec(dllimport)
#   define PIM_CDECL                __cdecl
#   define VEC_CALL                 __vectorcall
#   define pim_inline               static inline __forceinline
#   define pim_noalias              __restrict
#   define pim_alignas(x)           __declspec(align(x))
#   define pim_optimize             __pragma(optimize("gt", on))
#   define pim_deoptimize           __pragma(optimize("", off))
#   define pim_noreturn             __declspec(noreturn)
#endif // COMPILER_MSVC

#if COMPILER_CLANG
#   define INTERRUPT()              do { __asm("int3"); } while(0)
#   define SASSERT(x)               _Static_assert((x), #x)
#   define pim_thread_local         _Thread_local
#   define PIM_EXPORT               
#   define PIM_IMPORT               
#   define PIM_CDECL                
#   define VEC_CALL                 __vectorcall
#   define pim_inline               static inline __attribute__((always_inline))
#   define pim_noalias              __restrict__
#   define pim_alignas(x)           _Alignas(x)
#   define pim_optimize             _Pragma("clang optimize on")
#   define pim_deoptimize           _Pragma("clang optimize off")
#   define pim_noreturn             __declspec(noreturn)
#endif // COMPILER_CLANG

#if COMPILER_GCC
#   define INTERRUPT()              do { __asm("int3"); } while(0)
#   define SASSERT(x)               _Static_assert((x), #x)
#   define pim_thread_local         _Thread_local
#   define PIM_EXPORT               
#   define PIM_IMPORT               
#   define PIM_CDECL                
#   define VEC_CALL                 
#   define pim_inline               static inline __attribute__((always_inline))
#   define pim_noalias              __restrict__
#   define pim_alignas(x)           _Alignas(x)
#   define pim_optimize             _Pragma("GCC optimize \"O3\"")
#   define pim_deoptimize           _Pragma("GCC optimize \"O0\"")
#   define pim_noreturn             __declspec(noreturn)
#endif // COMPILER_GCC

#define _CAT_TOK(x, y)              x ## y
#define CAT_TOK(x, y)               _CAT_TOK(x, y)
#define _STR_TOK(x)                  #x
#define STR_TOK(x)                  _STR_TOK(x)

#define NELEM(x)                    ( sizeof(x) / sizeof((x)[0]) )
#define ARGS(x)                     (x), NELEM(x)
#define IF_TRUE(x, ...)             do { if (x) { __VA_ARGS__; } } while(0)
#define IF_FALSE(x, ...)            do { if (!(x)) { __VA_ARGS__; } } while(0)
#define pim_offsetof(T, f)          ((isize)&(((T*)0)->f))
#define pim_cast(T, p)              ( (T*)(p) )
#define pim_asbytes(p)              ( (u8*)(p) )
#define pim_alignof(x)              (_Alignof(x))
#define pim_min(a, b)               ((a) < (b) ? (a) : (b))
#define pim_max(a, b)               ((a) > (b) ? (a) : (b))
#define pim_lerp(a, b, t)           ((a) + ((b) - (a)) * (t))

#ifdef _DEBUG
#   define DEBUG_BUILD              1
#   define DEBUG_ONLY(...)          __VA_ARGS__
#else
#   define DEBUG_BUILD              0
#   define DEBUG_ONLY(...)           
#endif // def _DEBUG

#define RELEASE_BUILD               (!DEBUG_BUILD)
#if RELEASE_BUILD
#   define RELEASE_ONLY(...)        __VA_ARGS__
#else
#   define RELEASE_ONLY(...)        
#endif // RELEASE_BUILD

#define ASSERTS_ENABLED             (0 || DEBUG_BUILD)

#define PIM_FILELINE                CAT_TOK(__FILE__, STR_TOK(__LINE__))

#if ASSERTS_ENABLED
#   define ASSERT(x)                do { if (!(x)) { INTERRUPT(); } } while(0)
#   define CHECK(x)                 PimCheckFn((x))
#   define ASSERT_ONLY(...)         __VA_ARGS__
#else
#   define ASSERT(...)              
#   define CHECK(...)               __VA_ARGS__
#   define ASSERT_ONLY(...)         
#endif // ASSERTS_ENABLED

#if COMPILER_MSVC
// msvc has bloated headers (sal.h!)

#   ifndef NULL
#       ifdef __cplusplus
#           define NULL     0
#       else
#           define NULL     ((void*)0)
#       endif // cpp
#   endif // NULL

#   ifndef _STDBOOL
#       define _STDBOOL
#       define __bool_true_false_are_defined 1
#       ifndef __cplusplus
#           define bool     _Bool
#           define false    0
#           define true     1
#       endif // __cplusplus
#   endif // _STDBOOL

#   ifndef _VA_LIST_DEFINED
#       define _VA_LIST_DEFINED
        typedef char* va_list;
#   endif // _VA_LIST_DEFINED

    typedef signed char                 i8;
    typedef signed short                i16;
    typedef signed int                  i32;
    typedef signed long long            i64;
    typedef unsigned char               u8;
    typedef unsigned short              u16;
    typedef unsigned int                u32;
    typedef unsigned long long          u64;
    typedef i64                         isize;
    typedef u64                         usize;

#else
    #include <stddef.h>
    #include <stdint.h>
    #include <stdbool.h>
    #include <stdarg.h>

    typedef int8_t                      i8;
    typedef int16_t                     i16;
    typedef int32_t                     i32;
    typedef int64_t                     i64;
    typedef uint8_t                     u8;
    typedef uint16_t                    u16;
    typedef uint32_t                    u32;
    typedef uint64_t                    u64;
    typedef intptr_t                    isize;
    typedef uintptr_t                   usize;
#endif // COMPILER_MSVC

SASSERT(sizeof(i8) == 1);
SASSERT(sizeof(u8) == 1);
SASSERT(sizeof(i16) == 2);
SASSERT(sizeof(u16) == 2);
SASSERT(sizeof(i32) == 4);
SASSERT(sizeof(u32) == 4);
SASSERT(sizeof(i64) == 8);
SASSERT(sizeof(u64) == 8);

pim_inline bool PimCheckFn(bool cond) { if (!cond) { INTERRUPT(); } return cond; }

// assigns field and asserts that bitfield assignment does not overflow
#define ASSIGN(dst, src) do { (dst) = (src); ASSERT((dst) == (src)); } while(0)

#define PIM_PATH                    256
#define kMaxThreads                 64

typedef enum
{
    EAlloc_Perm = 0,
    EAlloc_Texture,
    EAlloc_Temp,
    EAlloc_Script,

    EAlloc_COUNT
} EAlloc;

typedef struct GenId_s
{
    u32 version : 8;
    u32 index : 24;
} GenId;

