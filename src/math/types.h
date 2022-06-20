#pragma once

#include "common/macro.h"

PIM_C_BEGIN

// ----------------------------------------------------------------------------

typedef enum
{
    Colorspace_Rec709,
    Colorspace_Rec2020,
    Colorspace_AP0,
    Colorspace_AP1,

    Colorspace_COUNT
} Colorspace;

typedef enum
{
    SGDist_Hemi = 0,
    SGDist_Sphere,

    SGDist_COUNT
} SGDist;

// ----------------------------------------------------------------------------

typedef struct pim_alignas(8) bool2_s
{
    pim_alignas_field(8) i32 x;
    i32 y;
} bool2;

typedef struct bool3_s
{
    i32 x;
    i32 y;
    i32 z;
} bool3;

typedef struct pim_alignas(16) bool4_s
{
    pim_alignas_field(16) i32 x;
    i32 y;
    i32 z;
    i32 w;
} bool4;

// ----------------------------------------------------------------------------

typedef struct pim_alignas(8) float2_s
{
    pim_alignas_field(8) float x;
    float y;
} float2;

typedef struct float3_s
{
    float x;
    float y;
    float z;
} float3;

typedef struct pim_alignas(16) float4_s
{
    pim_alignas_field(16) float x;
    float y;
    float z;
    float w;
} float4;

// ----------------------------------------------------------------------------

typedef struct pim_alignas(16) double2_s
{
    pim_alignas_field(16) double x;
    double y;
} double2;

typedef struct double3_s
{
    double x;
    double y;
    double z;
} double3;

typedef struct pim_alignas(16) double4_s
{
    pim_alignas_field(16) double x;
    double y;
    double z;
    double w;
} double4;

// ----------------------------------------------------------------------------

typedef struct pim_alignas(4) short2_s
{
    pim_alignas_field(4) i16 x;
    i16 y;
} short2;

typedef struct short3_s
{
    i16 x;
    i16 y;
    i16 z;
} short3;

typedef struct pim_alignas(8) short4_s
{
    pim_alignas_field(8) i16 x;
    i16 y;
    i16 z;
    i16 w;
} short4;

// ----------------------------------------------------------------------------

typedef struct pim_alignas(4) ushort2_s
{
    pim_alignas_field(4) u16 x;
    u16 y;
} ushort2;

typedef struct ushort3_s
{
    u16 x;
    u16 y;
    u16 z;
} ushort3;

typedef struct pim_alignas(8) ushort4_s
{
    pim_alignas_field(8) u16 x;
    u16 y;
    u16 z;
    u16 w;
} ushort4;

// ----------------------------------------------------------------------------

typedef struct pim_alignas(8) int2_s
{
    pim_alignas_field(8) i32 x;
    i32 y;
} int2;

typedef struct int3_s
{
    i32 x;
    i32 y;
    i32 z;
} int3;

typedef struct pim_alignas(16) int4_s
{
    pim_alignas_field(16) i32 x;
    i32 y;
    i32 z;
    i32 w;
} int4;

// ----------------------------------------------------------------------------

typedef struct pim_alignas(8) uint2_s
{
    pim_alignas_field(8) u32 x;
    u32 y;
} uint2;

typedef struct uint3_s
{
    u32 x;
    u32 y;
    u32 z;
} uint3;

typedef struct pim_alignas(16) uint4_s
{
    pim_alignas_field(16) u32 x;
    u32 y;
    u32 z;
    u32 w;
} uint4;

// ----------------------------------------------------------------------------

typedef struct pim_alignas(16) long2_s
{
    pim_alignas_field(16) i64 x;
    i64 y;
} long2;

typedef struct long3_s
{
    i64 x;
    i64 y;
    i64 z;
} long3;

typedef struct pim_alignas(16) long4_s
{
    pim_alignas_field(16) i64 x;
    i64 y;
    i64 z;
    i64 w;
} long4;

// ----------------------------------------------------------------------------

typedef struct pim_alignas(16) ulong2_s
{
    pim_alignas_field(16) u64 x;
    u64 y;
} ulong2;

typedef struct ulong3_s
{
    u64 x;
    u64 y;
    u64 z;
} ulong3;

typedef struct pim_alignas(32) ulong4_s
{
    pim_alignas_field(32) u64 x;
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

typedef struct R5G5B5A1_s
{
    u16 r : 5;
    u16 g : 5;
    u16 b : 5;
    u16 a : 1;
} R5G5B5A1_t;
SASSERT(sizeof(R5G5B5A1_t) == 2);

typedef struct R8G8B8A8_s
{
    u32 r : 8;
    u32 g : 8;
    u32 b : 8;
    u32 a : 8;
} R8G8B8A8_t;
SASSERT(sizeof(R8G8B8A8_t) == 4);

typedef struct A2R10G10B10_s
{
    u32 a : 2;
    u32 r : 10;
    u32 g : 10;
    u32 b : 10;
} A2R10G10B10_t;
SASSERT(sizeof(A2R10G10B10_t) == 4);

typedef struct pim_alignas(8) R16G16B16A16_s
{
    u32 r : 16;
    u32 g : 16;
    u32 b : 16;
    u32 a : 16;
} R16G16B16A16_t;
SASSERT(sizeof(R16G16B16A16_t) == 8);

// ----------------------------------------------------------------------------

typedef struct Ray_s
{
    float4 ro;
    float4 rd;
} Ray;

typedef struct Capsule_s
{
    // xyz: point 0
    // w: capsule radius
    float4 p0r;
    float4 p1;
} Capsule;

typedef struct Line3D_s
{
    float4 p0;
    float4 p1;
} Line3D;

typedef struct Line2D_s
{
    float2 p0;
    float2 p1;
} Line2D;

typedef struct Box3D_s
{
    float4 lo;
    float4 hi;
} Box3D;

typedef struct Box2D_s
{
    float2 lo;
    float2 hi;
} Box2D;

typedef struct Sphere_s
{
    // xyz: center
    // w: radius
    float4 value;
} Sphere;

typedef struct Circle_s
{
    float2 center;
    float radius;
} Circle;

typedef struct Plane3D_s
{
    // xyz: plane's normal
    // z: plane's distance from origin
    float4 value;
} Plane3D;

typedef struct Plane2D_s
{
    float2 normal;
    float distance;
} Plane2D;

typedef struct Tri3D_s
{
    float4 a;
    float4 b;
    float4 c;
} Tri3D;

typedef struct Tri2D_s
{
    float2 a;
    float2 b;
    float2 c;
} Tri2D;

// inward facing plane normals and positive distances
typedef struct Frustum_s
{
    Plane3D x0;
    Plane3D x1;
    Plane3D y0;
    Plane3D y1;
    Plane3D z0;
    Plane3D z1;
} Frustum;

typedef struct FrustumBasis_s
{
    float4 eye;
    float4 right;
    float4 up;
    float4 fwd;
    float2 slope;
    float zNear;
    float zFar;
} FrustumBasis;

typedef struct Grid_s
{
    Box3D bounds;
    int3 size;
    float cellsPerMeter;
} Grid;

// ----------------------------------------------------------------------------

typedef struct Dist1D_s
{
    float* pim_noalias pdf;
    float* pim_noalias cdf;
    u32* pim_noalias live;
    i32 length;
    float integral;
    u32 sum;
} Dist1D;

typedef struct dataset_s
{
    float* pim_noalias xs;
    float* pim_noalias ys;
    i32 len;
} dataset_t;

typedef struct pim_alignas(16) fit_s
{
    pim_alignas_field(16) float value[8];
} fit_t;

typedef struct BrdfLut_s
{
    int2 size;
    // r: reflectance, integral of Fc*D*V*NoL
    // g: visibility, integral of D*V*NoL
    float2* pim_noalias texels;
} BrdfLut;

// ----------------------------------------------------------------------------

typedef struct AmbCube_s
{
    // irradiance along normals of a cube
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

// ----------------------------------------------------------------------------

typedef struct SkyMedium_s
{
    float rCrust;   // crust radius
    float rAtmos;   // atmosphere radius, relative to crust

    float3 muR;     // rayleigh scattering coefficient (1 / mean free path), at sea level
    float rhoR;     // rayleigh density coefficient (1 / scale height)

    float muM;      // mie scattering coefficient (1 / mean free path), at sea level
    float rhoM;     // mie density coefficient (1 / scale height)
    float gM;       // mean cosine of mie phase function
} SkyMedium;

// ----------------------------------------------------------------------------

PIM_C_END
