#ifndef TRANSFORMS_H
#define TRANSFORMS_H

#include "shaders/quat.hlsl"

// https://docs.microsoft.com/en-us/windows/win32/direct3dgetstarted/work-with-shaders-and-shader-resources

// matrix indexing:
// m[r][c] in hlsl
// m[c][r] in glsl


// matrix-vector multiplication:
// v2.x = v1.x * m[0][0] + v1.y * m[0][1] + v1.z * m[0][2] + v1.w * m[0][3];
// v2.y = v1.x * m[1][0] + v1.y * m[1][1] + v1.z * m[1][2] + v1.w * m[1][3];
// v2.z = v1.x * m[2][0] + v1.y * m[2][1] + v1.z * m[2][2] + v1.w * m[2][3];
// v2.w = v1.x * m[3][0] + v1.y * m[3][1] + v1.z * m[3][2] + v1.w * m[3][3];

// simplifies to:
// v2.x = csum(v1 * m[0]);
// v2.y = csum(v1 * m[1]);
// v2.z = csum(v1 * m[2]);
// v2.w = csum(v1 * m[3]);

// or:
// v2 = mul(m, v1);


// Projection math for LH [0, 1] clip space
// m[0][0] = 1 / ((width / height) * tan(hfov / 2))
// m[1][1] = 1 / tan(hfov / 2)
// m[2][2] = far / (far - near)
// m[2][3] = -1 * (far * near) / (far - near)
// m[3][2] = 1

// resolves to:
// v2.x = v1.x / ((width / height) * tan(hfov / 2))
// v2.y = v1.y / tan(hfov / 2)
// v2.z = v1.z * far / (far - near) + v1.w * -(far * near) / (far - near)
// v2.w = v1.z * 1

// dividing by v2.w actually divides by v1.z

// just for reference, create this in host language
float4 CreateProjection(
    float width,
    float height,
    float hfov,
    float near,
    float far)
{
    float4 p;
    float aspect = width / height;
    float thfov2 = tan(hfov * 0.5f);
    float4 proj;
    p.x = 1.0f / (aspect * thfov2);
    p.y = 1.0f / thfov2;
    p.z = far / (far - near);
    p.w = (-far * near) / (far - near);
    return p;
}

// SRT
inline float3 ToWorld(float4 ts, float4 quat, float3 v)
{
    v = v * ts.w;
    v = q_rot(quat, v);
    v = v + ts.xyz;
    return v;
}

// -TR
inline float3 ToCamera(float4 ts, float4 quat, float3 v)
{
    v = v - ts.xyz;
    v = q_neg_rot(quat, v);
    return v;
}

// float4 proj = { m[0][0], m[1][1], m[2][2], m[2][3] };
// Assumes that v1 is a point (w = 1)
inline float4 ToClip(float4 proj, float3 v1)
{
    float4 v2;
    v2.x = v1.x * proj.x;
    v2.y = v1.y * proj.y;
    v2.z = v1.z * proj.z + proj.w;
    v2.w = v1.z;
    return v2;
}

inline float2 TransformUv(float4 uvTS, float2 uv)
{
    return uvTS.xy + uvTS.zw * uv;
}

#endif // TRANSFORMS_H
