#pragma once

#include "common/macro.h"

PIM_C_BEGIN

// ----------------------------------------------------------------------------

typedef pim_alignas(8) struct bool2_s
{
    i32 x;
    i32 y;
} bool2;

typedef struct bool3_s
{
    i32 x;
    i32 y;
    i32 z;
} bool3;

typedef pim_alignas(16) struct bool4_s
{
    i32 x;
    i32 y;
    i32 z;
    i32 w;
} bool4;

// ----------------------------------------------------------------------------

typedef pim_alignas(8) struct float2_s
{
    float x;
    float y;
} float2;

typedef struct float3_s
{
    float x;
    float y;
    float z;
} float3;

typedef pim_alignas(16) struct float4_s
{
    float x;
    float y;
    float z;
    float w;
} float4;

// ----------------------------------------------------------------------------

typedef pim_alignas(16) struct double2_s
{
    double x;
    double y;
} double2;

typedef struct double3_s
{
    double x;
    double y;
    double z;
} double3;

typedef pim_alignas(32) struct double4_s
{
    double x;
    double y;
    double z;
    double w;
} double4;

// ----------------------------------------------------------------------------

typedef pim_alignas(4) struct short2_s
{
    i16 x;
    i16 y;
} short2;

typedef struct short3_s
{
    i16 x;
    i16 y;
    i16 z;
} short3;

typedef pim_alignas(8) struct short4_s
{
    i16 x;
    i16 y;
    i16 z;
    i16 w;
} short4;

// ----------------------------------------------------------------------------

typedef pim_alignas(4) struct ushort2_s
{
    u16 x;
    u16 y;
} ushort2;

typedef struct ushort3_s
{
    u16 x;
    u16 y;
    u16 z;
} ushort3;

typedef pim_alignas(8) struct ushort4_s
{
    u16 x;
    u16 y;
    u16 z;
    u16 w;
} ushort4;

// ----------------------------------------------------------------------------

typedef pim_alignas(8) struct int2_s
{
    i32 x;
    i32 y;
} int2;

typedef struct int3_s
{
    i32 x;
    i32 y;
    i32 z;
} int3;

typedef pim_alignas(16) struct int4_s
{
    i32 x;
    i32 y;
    i32 z;
    i32 w;
} int4;

// ----------------------------------------------------------------------------

typedef pim_alignas(8) struct uint2_s
{
    u32 x;
    u32 y;
} uint2;

typedef struct uint3_s
{
    u32 x;
    u32 y;
    u32 z;
} uint3;

typedef pim_alignas(16) struct uint4_s
{
    u32 x;
    u32 y;
    u32 z;
    u32 w;
} uint4;

// ----------------------------------------------------------------------------

typedef pim_alignas(16) struct long2_s
{
    i64 x;
    i64 y;
} long2;

typedef struct long3_s
{
    i64 x;
    i64 y;
    i64 z;
} long3;

typedef pim_alignas(32) struct long4_s
{
    i64 x;
    i64 y;
    i64 z;
    i64 w;
} long4;

// ----------------------------------------------------------------------------

typedef pim_alignas(16) struct ulong2_s
{
    u64 x;
    u64 y;
} ulong2;

typedef struct ulong3_s
{
    u64 x;
    u64 y;
    u64 z;
} ulong3;

typedef pim_alignas(32) struct ulong4_s
{
    u64 x;
    u64 y;
    u64 z;
    u64 w;
} ulong4;

// ----------------------------------------------------------------------------

typedef struct quat_s
{
    float4 v;
} quat;

typedef struct float2x2_s
{
    float2 c0;
    float2 c1;
} float2x2;

typedef struct float3x3_s
{
    float4 c0;
    float4 c1;
    float4 c2;
} float3x3;

typedef struct float4x4_s
{
    float4 c0;
    float4 c1;
    float4 c2;
    float4 c3;
} float4x4;

typedef struct float4x2_s
{
    float4 c0;
    float4 c1;
} float4x2;

// ----------------------------------------------------------------------------

typedef struct int2x2_s
{
    int2 c0;
    int2 c1;
} int2x2;

typedef struct int3x3_s
{
    int4 c0;
    int4 c1;
    int4 c2;
} int3x3;

typedef struct int4x4_s
{
    int4 c0;
    int4 c1;
    int4 c2;
    int4 c3;
} int4x4;

// ----------------------------------------------------------------------------

typedef struct ray_s
{
    float4 ro;
    float4 rd;
} ray_t;

typedef struct capsule_s
{
    // xyz: point 0
    // w: capsule radius
    float4 p0r;
    float4 p1;
} capsule_t;

typedef struct line_s
{
    float4 p0;
    float4 p1;
} line_t;

typedef struct line2d_s
{
    float2 p0;
    float2 p1;
} line2d_t;

typedef struct box_s
{
    float4 lo;
    float4 hi;
} box_t;

typedef struct box2d_s
{
    float2 lo;
    float2 hi;
} box2d_t;

typedef struct sphere_s
{
    // xyz: center
    // w: radius
    float4 value;
} sphere_t;

typedef struct circle_s
{
    float2 center;
    float radius;
} circle_t;

typedef struct plane_s
{
    // xyz: plane's normal
    // z: plane's distance from origin
    float4 value;
} plane_t;

typedef struct plane2d_s
{
    float2 normal;
    float distance;
} plane2d_t;

typedef struct tri_s
{
    float4 a;
    float4 b;
    float4 c;
} tri_t;

typedef struct tri2d_s
{
    float2 a;
    float2 b;
    float2 c;
} tri2d_t;

// inward facing plane normals and positive distances
typedef struct frus_s
{
    plane_t x0;
    plane_t x1;
    plane_t y0;
    plane_t y1;
    plane_t z0;
    plane_t z1;
} frus_t;

typedef struct frusbasis_s
{
    float4 eye;
    float4 right;
    float4 up;
    float4 fwd;
    float2 slope;
    float zNear;
    float zFar;
} frusbasis_t;

typedef struct AmbCube_s
{
    // radiance along normals of a cube
    float4 Values[6];
} AmbCube_t;

typedef struct SH4s
{
    float v[4];
} SH4s;

typedef struct SH4v
{
    float3 v[4];
} SH4v;

PIM_C_END
