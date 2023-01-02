#ifndef R_GPU_SHARED_H
#define R_GPU_SHARED_H

#ifndef PIM_HLSL
#   include "rendering/r_config.h"
#   include "math/types.h"
#endif // !PIM_HLSL

PIM_STRUCT_BEGIN(GpuGlobals)
{
    float4x4 worldToClip;

    float4 eye;
    float4 forward;
    float4 right;
    float4 up;
    float2 slope;
    float zNear;
    float zFar;

    float hdrEnabled;
    float whitepoint;
    float displayNits;
    float uiNits;

    uint2 renderSize;
    uint2 displaySize;
} PIM_STRUCT_END(GpuGlobals);

#ifndef PIM_HLSL
    extern GpuGlobals g_GpuGlobals;
#endif // !PIM_HLSL

#endif // R_GPU_SHARED_H
