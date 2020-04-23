#include "common/pimcpy.h"
#include "common/unroll.h"

void pimcpy(
    void* pim_noalias dst,
    const void* pim_noalias src,
    i32 len)
{
    ASSERT(!len || (dst && src));
    ASSERT(len >= 0);
    ASSERT(src != dst);

    const usize align = (usize)dst | (usize)src | len;
    i32 n16 = 0;
    i32 n4 = 0;
    Unroll16(len, &n16, &n4);

    if (!(align & 3))
    {
        u32* pim_noalias dst4 = dst;
        const u32* pim_noalias src4 = src;
        while (n16--)
        {
            *dst4++ = *src4++;
            *dst4++ = *src4++;
            *dst4++ = *src4++;
            *dst4++ = *src4++;
        }
        while (n4--)
        {
            *dst4++ = *src4++;
        }
    }
    else
    {
        u8* pim_noalias dst1 = dst;
        const u8* pim_noalias src1 = src;
        i32 n1 = len & 3;
        n4 += n16 * 4;
        while (n4--)
        {
            *dst1++ = *src1++;
            *dst1++ = *src1++;
            *dst1++ = *src1++;
            *dst1++ = *src1++;
        }
        while (n1--)
        {
            *dst1++ = *src1++;
        }
    }
}

void pimmove(void* dst, const void* src, i32 len)
{
    ASSERT(!len || (dst && src));
    ASSERT(len >= 0);
    ASSERT(src != dst);

    if (dst < src)
    {
        pimcpy(dst, src, len);
    }
    else
    {
        const usize align = (usize)dst | (usize)src | len;
        if (!(align & 3))
        {
            u32* dst4 = dst;
            const u32* src4 = src;
            len >>= 2;
            for (i32 i = len - 1; i >= 0; --i)
            {
                dst4[i] = src4[i];
            }
        }
        else
        {
            u8* dst1 = dst;
            const u8* src1 = src;
            for (i32 i = len - 1; i >= 0; --i)
            {
                dst1[i] = src1[i];
            }
        }
    }
}

void pimset(void* dst, u32 val, i32 len)
{
    ASSERT(!len || dst);
    ASSERT(len >= 0);

    val &= 0xff;
    val = val | (val << 8) | (val << 16) | (val << 24);
    const usize align = (usize)dst | len;

    i32 n16 = 0;
    i32 n4 = 0;
    Unroll16(len, &n16, &n4);

    if (!(align & 3))
    {
        u32* dst4 = dst;
        while (n16--)
        {
            *dst4++ = val;
            *dst4++ = val;
            *dst4++ = val;
            *dst4++ = val;
        }
        while (n4--)
        {
            *dst4++ = val;
        }
    }
    else
    {
        i32 n1 = len & 3;
        n4 += n16 * 4;
        u8* dst1 = dst;
        const u8 val1 = val;
        while (n4--)
        {
            *dst1++ = val1;
            *dst1++ = val1;
            *dst1++ = val1;
            *dst1++ = val1;
        }
        while (n1--)
        {
            *dst1++ = val1;
        }
    }
}

i32 pimcmp(
    const void* pim_noalias lhs,
    const void* pim_noalias rhs,
    i32 len)
{
    ASSERT(!len || (lhs && rhs));
    ASSERT(len >= 0);
    if (lhs == rhs)
    {
        return 0;
    }

    const usize align = (usize)lhs | (usize)rhs | len;
    i32 n16 = 0;
    i32 n4 = 0;
    Unroll16(len, &n16, &n4);

    if (!(align & 3))
    {
        const u32* pim_noalias lhs4 = lhs;
        const u32* pim_noalias rhs4 = rhs;

        while (n16--)
        {
            u32 c =
                (lhs4[0] - rhs4[0]) |
                (lhs4[1] - rhs4[1]) |
                (lhs4[2] - rhs4[2]) |
                (lhs4[3] - rhs4[3]);
            if (c)
            {
                n4 += 4;
                break;
            }
            lhs4 += 4;
            rhs4 += 4;
        }

        while (n4--)
        {
            u32 a = *lhs4++;
            u32 b = *rhs4++;
            if (a != b)
            {
                return a < b;
            }
        }
    }
    else
    {
        const u8* pim_noalias lhs1 = lhs;
        const u8* pim_noalias rhs1 = rhs;
        n4 += n16 * 4;
        i32 n1 = len & 3;

        while (n4--)
        {
            u32 c =
                (lhs1[0] - rhs1[0]) |
                (lhs1[1] - rhs1[1]) |
                (lhs1[2] - rhs1[2]) |
                (lhs1[3] - rhs1[3]);
            if (c)
            {
                n1 += 4;
                break;
            }
            lhs1 += 4;
            rhs1 += 4;
        }

        while (n1--)
        {
            i32 c = *lhs1++ - *rhs1++;
            if (c)
            {
                return c;
            }
        }
    }

    return 0;
}
