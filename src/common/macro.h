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
#define EAssert()                   IfTrue(errno, Interrupt())
#define DebugAssert(x)              IfDebug(Assert(x))
#define DebugEAssert()              IfDebug(EAssert())

#define Min(a, b)                   ( (a) < (b) ? (a) : (b) )
#define Max(a, b)                   ( (a) > (b) ? (a) : (b) )
#define Clamp(x, lo, hi)            ( Min(hi, Max(lo, x)) )
#define Lerp(a, b, t)               ( (a) + (((b) - (a)) * (t)) )

