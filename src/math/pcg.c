#include "math/pcg.h"
#include "common/stringutil.h"

#include <string.h>

u32 VEC_CALL Pcg1_Bytes(const void* pim_noalias x, i32 bytes, u32 hash)
{
    if (x)
    {
        const u8* pim_noalias src = x;
        while (bytes >= sizeof(u32) * 16)
        {
            u32 v[16];
            memcpy(&v, src, sizeof(v));
            for (i32 i = 0; i < NELEM(v); ++i)
            {
                hash = Pcg1_Lcg(hash) + v[i];
                hash = Pcg1_Permute(hash);
            }
            src += sizeof(v);
            bytes -= sizeof(v);
        }
        while (bytes >= sizeof(u32))
        {
            u32 v;
            memcpy(&v, src, sizeof(v));
            hash = Pcg1_Lcg(hash) + v;
            hash = Pcg1_Permute(hash);
            src += sizeof(v);
            bytes -= sizeof(v);
        }
        if (bytes > 0)
        {
            u32 v = 0;
            memcpy(&v, src, bytes);
            hash = Pcg1_Lcg(hash) + v;
            hash = Pcg1_Permute(hash);
        }
    }
    return hash;
}

u32 VEC_CALL Pcg1_String(const char* pim_noalias x, u32 hash)
{
    return Pcg1_Bytes(x, StrLen(x), hash);
}

uint2 VEC_CALL Pcg2_Bytes(const void* pim_noalias x, i32 bytes, uint2 hash)
{
    if (x)
    {
        const u8* pim_noalias src = x;
        while (bytes >= sizeof(uint2) * 8)
        {
            uint2 v[8];
            memcpy(&v, src, sizeof(v));
            for (i32 i = 0; i < NELEM(v); ++i)
            {
                hash = Pcg2_Lcg(hash);
                hash.x += v[i].x;
                hash.y += v[i].y;
                hash = Pcg2_Permute(hash);
            }
            src += sizeof(v);
            bytes -= sizeof(v);
        }
        while (bytes >= sizeof(uint2))
        {
            uint2 v;
            memcpy(&v, src, sizeof(v));
            hash = Pcg2_Lcg(hash);
            hash.x += v.x;
            hash.y += v.y;
            hash = Pcg2_Permute(hash);
            src += sizeof(v);
            bytes -= sizeof(v);
        }
        if (bytes > 0)
        {
            uint2 v = { 0 };
            memcpy(&v, src, bytes);
            hash = Pcg2_Lcg(hash);
            hash.x += v.x;
            hash.y += v.y;
            hash = Pcg2_Permute(hash);
        }
    }
    return hash;
}

uint2 VEC_CALL Pcg2_String(const char* pim_noalias x, uint2 hash)
{
    return Pcg2_Bytes(x, StrLen(x), hash);
}

uint3 VEC_CALL Pcg3_Bytes(const void* pim_noalias x, i32 bytes, uint3 hash)
{
    if (x)
    {
        const u8* pim_noalias src = x;
        while (bytes >= sizeof(uint3) * 5)
        {
            uint3 v[5];
            memcpy(&v, src, sizeof(v));
            for (i32 i = 0; i < NELEM(v); ++i)
            {
                hash = Pcg3_Lcg(hash);
                hash.x += v[i].x;
                hash.y += v[i].y;
                hash.z += v[i].z;
                hash = Pcg3_Permute(hash);
            }
            src += sizeof(v);
            bytes -= sizeof(v);
        }
        while (bytes >= sizeof(uint3))
        {
            uint3 v;
            memcpy(&v, src, sizeof(v));
            hash = Pcg3_Lcg(hash);
            hash.x += v.x;
            hash.y += v.y;
            hash.z += v.z;
            hash = Pcg3_Permute(hash);
            src += sizeof(v);
            bytes -= sizeof(v);
        }
        if (bytes > 0)
        {
            uint3 v = { 0 };
            memcpy(&v, src, bytes);
            hash = Pcg3_Lcg(hash);
            hash.x += v.x;
            hash.y += v.y;
            hash.z += v.z;
            hash = Pcg3_Permute(hash);
        }
    }
    return hash;
}

uint3 VEC_CALL Pcg3_String(const char* pim_noalias x, uint3 hash)
{
    return Pcg3_Bytes(x, StrLen(x), hash);
}

uint4 VEC_CALL Pcg4_Bytes(const void* pim_noalias x, i32 bytes, uint4 hash)
{
    if (x)
    {
        const u8* pim_noalias src = x;
        while (bytes >= sizeof(uint4) * 4)
        {
            uint4 v[4];
            memcpy(&v, src, sizeof(v));
            for (i32 i = 0; i < NELEM(v); ++i)
            {
                hash = Pcg4_Lcg(hash);
                hash.x += v[i].x;
                hash.y += v[i].y;
                hash.z += v[i].z;
                hash.w += v[i].w;
                hash = Pcg4_Permute(hash);
            }
            src += sizeof(v);
            bytes -= sizeof(v);
        }
        while (bytes >= sizeof(uint4))
        {
            uint4 v;
            memcpy(&v, src, sizeof(v));
            hash = Pcg4_Lcg(hash);
            hash.x += v.x;
            hash.y += v.y;
            hash.z += v.z;
            hash.w += v.w;
            hash = Pcg4_Permute(hash);
            src += sizeof(v);
            bytes -= sizeof(v);
        }
        if (bytes > 0)
        {
            uint4 v = { 0 };
            memcpy(&v, src, bytes);
            hash = Pcg4_Lcg(hash);
            hash.x += v.x;
            hash.y += v.y;
            hash.z += v.z;
            hash.w += v.w;
            hash = Pcg4_Permute(hash);
        }
    }
    return hash;
}

uint4 VEC_CALL Pcg4_String(const char* pim_noalias x, uint4 hash)
{
    return Pcg4_Bytes(x, StrLen(x), hash);
}
