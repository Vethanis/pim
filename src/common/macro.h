#pragma once

#if _DEBUG
    #define IfDebug(x) x
#else
    #define IfDebug(x) 
#endif // _DEBUG

#ifdef _MSC_VER
    #define IfWin32(x) x
#else
    #define IfWin32(x) 
#endif // _MSC_VER

#define DllImport                   IfWin32(__declspec( dllimport ))
#define DllExport                   IfWin32(__declspec( dllexport ))
#define CDecl                       IfWin32(__cdecl)

#define CountOf(x)                  ( sizeof(x) / sizeof((x)[0]) )
#define OffsetOf(s, m)              ( (size_t)&(((s*)0)->m) )
#define Interrupt()                 __debugbreak()
#define DebugInterrupt()            IfDebug(Interrupt())
#define IfTrue(x, expr)             if(x) { expr; }
#define IfFalse(x, expr)            if(!(x)) { expr; }
#define Assert(x)                   IfFalse(x, Interrupt())
#define EAssert()                   IfTrue(IO::GetErrNo(), Interrupt(); IO::ClearErrNo())
#define DebugAssert(x)              IfDebug(Assert(x))
#define DebugEAssert()              IfDebug(EAssert())
#define CheckE0()                   IfTrue(IO::GetErrNo(), DebugInterrupt(); IO::ClearErrNo(); return 0)
#define CheckE(expr)                IfTrue(IO::GetErrNo(), DebugInterrupt(); IO::ClearErrNo(); expr)
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

typedef char* VaList;
#define VaStart(arg)                (reinterpret_cast<VaList>(&(arg)) + PtrSizeOf(arg))
#define VaArg(va, T)                *reinterpret_cast<T*>((va += PtrSizeOf(T)) - PtrSizeOf(T))

#define PIM_MAX_PATH                256

