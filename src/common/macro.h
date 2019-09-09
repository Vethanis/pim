#pragma once

#if _DEBUG
    #define IfDebug(x) x
#else
    #define IfDebug(x) 
#endif // _DEBUG

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

#define RootDir                     ".."
#define PacksDir                    "../packs"
