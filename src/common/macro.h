#pragma once

#if _DEBUG
    #define IfDebug(x) x
#else
    #define IfDebug(x) 
#endif // _DEBUG

#define CountOf(x)                  ( sizeof(x) / sizeof((x)[0]) )
#define Break()                     __debugbreak()
#define IfTrue(x, expr)             { if( (x)) { expr; } }
#define IfFalse(x, expr)            { if(!(x)) { expr; } }
#define Assert(x)                   IfFalse(x, Break())
#define DebugAssert(x)              IfDebug(Assert(x))
