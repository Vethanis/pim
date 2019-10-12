#include "common/hashstring.h"
#include "containers/dict.h"
#include "common/text.h"

#if _DEBUG

using HashText = Text<64>;

using HashStorage = DictTable<16, HashStringKey, HashText>;

static HashStorage ms_store[HashNSCount];

static HashStorage& GetStore(HashStringKey key)
{
    return ms_store[key & HashNSMask];
}

cstr HashString::Lookup(HashStringKey key)
{
    cstr result = 0;
    HashText* pText = GetStore(key).get(key);
    if (pText)
    {
        result = pText->value;
    }
    return result;
}

void HashString::Insert(HashNamespace ns, cstrc value)
{
    if (!value || !value[0])
    {
        return;
    }

    HashStringKey key = StrHash(ns, value);
    HashText& dst = GetStore(key)[key];

    if (dst.value[0])
    {
        if (!StrICmp(argof(dst.value), value))
        {
            return;
        }
        DebugInterrupt();
        return;
    }

    StrCpy(argof(dst.value), value);
}

#endif // _DEBUG
