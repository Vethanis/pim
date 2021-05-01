#ifndef MACRO_HLSL
#define MACRO_HLSL

#define PIM_HLSL        1
#define i32             int
#define u32             uint

#include "../rendering/r_config.h"

// TODO: move these to shared config file
#define kMinLightDist   0.01
#define kMinLightDistSq 0.001
#define kMinAlpha       0.00001525878
#define kPi             3.141592653
#define kTau            6.283185307
#define kEpsilon        2.38418579e-7

#ifndef FLT_MAX
#   define FLT_MAX 3.402823e+38
#endif // FLT_MAX
#ifndef FLT_MIN
#   define FLT_MIN 1.175494e-38
#endif // FLT_MIN

#define dotsat(a, b)    saturate(dot((a), (b)))
#define unlerp(a, b, x) saturate(((x) - (a)) / ((b) - (a)))

#include "bindings.hlsl"

#endif // MACRO_HLSL
