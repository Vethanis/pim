#pragma once

#include "math/vec_types.h"

struct alignas(16) Plane
{
    float3 normal;
    float distance;
};

struct alignas(16) Sphere
{
    float3 center;
    float radius;
};

struct Box
{
    float3 center;
    float3 extents;
};

struct Cone
{
    float3 center;
    float2 angle; // sincos of cone angle
};

struct Line
{
    // two points on the line
    float3 p0;
    float3 p1;
};

struct LineSegment
{
    float3 start;
    float3 stop;
};

struct Capsule
{
    float3 a;
    float3 b;
    float radius;
};

struct VerticalCapsule
{
    float3 base;
    float height;
    float radius;
};

struct Triangle
{
    float3 a;
    float3 b;
    float3 c;
};

struct Quad
{
    float3 a;
    float3 b;
    float3 c;
    float3 d;
};

enum FrustumPlane : u8
{
    FrustumPlane_Right, // +X
    FrustumPlane_Left,  // -X
    FrustumPlane_Up,    // +Y
    FrustumPlane_Down,  // -Y
    FrustumPlane_Near,  // +Z
    FrustumPlane_Far,   // -Z

    FrustumPlane_COUNT
};

struct Frustum
{
    Plane planes[FrustumPlane_COUNT];
};

struct Ray
{
    float3 origin;
    float3 direction;
};
