#ifndef COMMON_HLSL
#define COMMON_HLSL

#define PIM_HLSL                1
#define PIM_STRUCT_BEGIN(name)  struct name
#define PIM_STRUCT_END(name)    
#define i32                     int
#define u32                     uint

#include "../rendering/r_config.h"
#include "../rendering/r_gpu_shared.h"
#include "bindings.hlsl"

// TODO: move these to shared config file
#define kMinLightDist   0.01f
#define kMinLightDistSq 0.001f
#define kMinAlpha       0.00001525878f
#define kPi             3.141592653f
#define kTau            6.283185307f
#define kEpsilon        (1.0f / (1 << 23))
#define kLog2Epsilon    (-23.0f)

#ifndef FLT_MAX
#   define FLT_MAX 3.402823e+38f
#endif // FLT_MAX
#ifndef FLT_MIN
#   define FLT_MIN 1.175494e-38f
#endif // FLT_MIN

#define dotsat(a, b)    saturate(dot((a), (b)))
#define unlerp(a, b, x) saturate(((x) - (a)) / ((b) - (a)))

#endif // COMMON_HLSL
