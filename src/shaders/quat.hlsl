#ifndef QUAT_H
#define QUAT_H

// ----------------------------------------------------------------------------
// quaternion math

inline float4 q_new()
{
    return float4(0.0f, 0.0f, 0.0f, 1.0f);
}

inline float4 q_vec(float3 v)
{
    return float4(v, 0.0f);
}

inline float4 q_mul(float4 q1, float4 q2)
{
    float3 a = (q2.xyz * q1.w) + (q1.xyz * q2.w) + cross(q1.xyz, q2.xyz);
    float b = (q1.w * q2.w) - dot(q1.xyz, q2.xyz);
    return float4(a, b);
}

inline float4 q_conj(float4 q)
{
    return float4(-q.x, -q.y, -q.z, q.w);
}

inline float4 q_inv(float4 q)
{
    float4 conj = q_conj(q);
    return conj * rcp(dot(q, q));
}

inline float3 q_rot(float4 q, float3 v)
{
    return q_mul(q, q_mul(q_vec(v), q_conj(q))).xyz;
}

inline float3 q_neg_rot(float4 q, float3 v)
{
    q.w = -q.w;
    return q_rot(q, v);
}

#endif // QUAT_H
