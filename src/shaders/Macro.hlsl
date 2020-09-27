#ifndef MACRO_HLSL
#define MACRO_HLSL

#define i32             int
#define u32             uint
#define kMinLightDist   0.01
#define kMinLightDistSq 0.001
#define kMinAlpha       0.00001525878
#define kPi             3.141592653
#define kTau            6.283185307
#define kEpsilon        2.38418579e-7

#define dotsat(a, b)    saturate(dot((a), (b)))
#define unlerp(a, b, x) saturate(((x) - (a)) / ((b) - (a)))

#endif // MACRO_HLSL
