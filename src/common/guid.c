#include "common/guid.h"
#include "common/fnv1a.h"
#include "common/random.h"
#include "common/stringutil.h"
#include "common/time.h"
#include "common/atomics.h"
#include "containers/dict.h"
#include "containers/text.h"

static Dict ms_guidToStr =
{
    .keySize = sizeof(Guid),
    .valueSize = sizeof(Text16),
};
static u64 ms_counter;

bool Guid_IsNull(Guid x)
{
    return !(x.a | x.b);
}

bool Guid_Equal(Guid lhs, Guid rhs)
{
    return !((lhs.a - rhs.a) | (lhs.b - rhs.b));
}

i32 Guid_Compare(Guid lhs, Guid rhs)
{
    if (lhs.a != rhs.a)
    {
        return lhs.a < rhs.a ? -1 : 1;
    }
    if (lhs.b != rhs.b)
    {
        return lhs.b < rhs.b ? -1 : 1;
    }
    return 0;
}

Guid Guid_New(void)
{
    u64 a = Time_Now();
    a = a ? a : 1;
    u64 b = inc_u64(&ms_counter, MO_Relaxed);
    return (Guid) { a, b };
}

void Guid_SetName(Guid id, const char* str)
{
    if (str && str[0])
    {
        Text16 text = { 0 };
        StrCpy(ARGS(text.c), str);
        if (!Dict_Add(&ms_guidToStr, &id, &text))
        {
            Dict_Set(&ms_guidToStr, &id, &text);
        }
    }
    else
    {
        Dict_Rm(&ms_guidToStr, &id, NULL);
    }
}

bool Guid_GetName(Guid id, char* dst, i32 size)
{
    Text16 text = { 0 };
    if (Dict_Get(&ms_guidToStr, &id, &text))
    {
        StrCpy(dst, size, text.c);
        return true;
    }
    dst[0] = 0;
    return false;
}

i32 Guid_Find(Guid const *const pim_noalias ptr, i32 count, Guid key)
{
    ASSERT(ptr || !count);
    ASSERT(count >= 0);
    for (i32 i = 0; i < count; ++i)
    {
        if (Guid_Equal(ptr[i], key))
        {
            return i;
        }
    }
    return -1;
}

Guid Guid_HashStr(char const *const pim_noalias str)
{
    Guid id = { 0 };
    if (str && str[0])
    {
        id.a = Fnv64String(str, Fnv64Bias);
        id.b = Fnv64String(str, id.a);
        id.a = id.a ? id.a : 1;
    }
    return id;
}

Guid Guid_FromStr(char const *const pim_noalias str)
{
    Guid id = Guid_HashStr(str);
    if (id.a)
    {
        Guid_SetName(id, str);
    }
    return id;
}

Guid Guid_FromBytes(void const *const pim_noalias ptr, i32 nBytes)
{
    Guid id = { 0 };
    if (ptr && nBytes > 0)
    {
        id.a = Fnv64Bytes(ptr, nBytes, Fnv64Bias);
        id.b = Fnv64Bytes(ptr, nBytes, id.a);
        id.a = id.a ? id.a : 1;
    }
    return id;
}

void Guid_Format(char* dst, i32 size, Guid value)
{
    u32 a0 = (value.a >> 32) & 0xffffffff;
    u32 a1 = (value.a >> 0) & 0xffffffff;
    u32 b0 = (value.b >> 32) & 0xffffffff;
    u32 b1 = (value.b >> 0) & 0xffffffff;
    StrCatf(dst, size, "%x_%x_%x_%x", a0, a1, b0, b1);
}

u32 Guid_HashOf(Guid x)
{
    return Fnv32Qword(x.b, Fnv32Qword(x.a, Fnv32Bias));
}
