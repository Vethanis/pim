#pragma once

// ----------------------------------------------------------------------------

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
    #endif // def TARGET_IPHONE_SIMULATOR
#elif defined(__linux__)
    #define CUR_PLAT                    PLAT_LINUX
#else
    #define CUR_PLAT                    PLAT_UNKNOWN
    #error Unable to detect current platform
#endif // def _WIN32 || def __CYGWIN__

#if CUR_PLAT == PLAT_WINDOWS
    #define inline                  __forceinline
    #define Restrict                __restrict
    #define DllImport               __declspec( dllimport )
    #define DllExport               __declspec( dllexport )
    #define CDecl                   __cdecl
    #define Interrupt()             __debugbreak()
    #define IfWin32(x)              x
    #define IfUnix(x)               
#else
    #define IfWin32(x)              
    #define IfUnix(x)               x
#endif // PLAT_WINDOWS

// ----------------------------------------------------------------------------

#define TARGET_UNKNOWN                  0
#define TARGET_DEBUG                    1
#define TARGET_RELEASE                  2
#define TARGET_MASTER                   3

#ifdef _DEBUG
    #define CUR_TARGET                  TARGET_DEBUG
    #define IfDebug(x)                  x
#else
    #define IfDebug(x)                  
#endif // def _DEBUG

#ifdef NDEBUG
    #define CUR_TARGET                  TARGET_RELEASE
    #define IfRelease(x)                x
#else
    #define IfRelease(x)                
#endif // def _NDEBUG

#ifdef _MASTER
    #define CUR_TARGET                  TARGET_MASTER
    #define IfMaster(x)                 x
#else
    #define IfMaster(x)                 
#endif // def _MASTER

#ifndef CUR_TARGET
    #define CUR_TARGET                  TARGET_UNKNOWN
    #error Unable to detect current build target
#endif // ndef CUR_TARGET

// ----------------------------------------------------------------------------

#define countof(x)                  ( sizeof(x) / sizeof((x)[0]) )
#define argof(x)                    x, countof(x)
#define OffsetOf(s, m)              ( (usize)&(((s*)0)->m) )
#define DebugInterrupt()            IfDebug(Interrupt())
#define IfTrue(x, expr)             if(x) { expr; }
#define IfFalse(x, expr)            if(!(x)) { expr; }
#define Assert(x)                   IfFalse(x, Interrupt())
#define DebugAssert(x)              IfDebug(Assert(x))
#define Check0(x)                   IfFalse(x, DebugInterrupt(); return 0)
#define Check(x, expr)              IfFalse(x, DebugInterrupt(); expr)

#define Min(a, b)                   ( (a) < (b) ? (a) : (b) )
#define Max(a, b)                   ( (a) > (b) ? (a) : (b) )
#define Clamp(x, lo, hi)            ( Min(hi, Max(lo, x)) )
#define Lerp(a, b, t)               ( (a) + (((b) - (a)) * (t)) )

#define AlignGrow(x, y)             ( ((x) + (y) - 1u) & ~((y) - 1u) )
#define AlignGrowT(T, U)            AlignGrow(sizeof(T), sizeof(U))
#define DWordSizeOf(T)              AlignGrowT(T, u32)
#define QWordSizeOf(T)              AlignGrowT(T, u64)
#define PtrSizeOf(T)                AlignGrowT(T, usize)
#define CacheSizeOf(T)              AlignGrow(sizeof(T), 64u)
#define PageSizeOf(T)               AlignGrow(sizeof(T), 4096u)
#define PIM_MAX_PATH                256

// ----------------------------------------------------------------------------

typedef char* VaList;
#define VaStart(arg)                (reinterpret_cast<VaList>(&(arg)) + PtrSizeOf(arg))
#define VaArg(va, T)                *reinterpret_cast<T*>((va += PtrSizeOf(T)) - PtrSizeOf(T))

// ----------------------------------------------------------------------------

