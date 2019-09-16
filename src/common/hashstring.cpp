#include "common/hashstring.h"
#include "containers/dict.h"

using HashStorage = DictTable<16, HashStringKey, Slice<char> >;

static HashStorage ms_store[HashNSCount];
u8 HashString::NSIdx;

Slice<char> HashString::Lookup(HashStringKey key)
{
    Slice<char> result = { 0, 0 };
    Slice<char>* pSlice = ms_store[key & HashNSMask].get(key >> HashNSBits);
    if (pSlice)
    {
        result = *pSlice;
    }
    return result;
}

HashStringKey HashString::Insert(HashNamespace ns, cstrc value)
{
    if (!value)
    {
        return 0;
    }

    HashStringKey key = StrHash(ns, value);
    Slice<char>& dst = ms_store[ns][key >> HashNSBits];

    if (dst.begin())
    {
        if (!StrICmp(dst.begin(), value))
        {
            return key;
        }
        DebugInterrupt();
        return 0;
    }

    const isize len = StrLen(value) + 1;
    dst = Allocator::Alloc<char>(len);
    StrCpy(dst.begin(), dst.size(), value);

    return key;
}

void HashString::Reset()
{
    for (HashStorage& store : ms_store)
    {
        for (auto& dict : store.m_dicts)
        {
            for (Slice<char>& value : dict.m_values)
            {
                Allocator::Free(value);
            }
        }
        store.reset();
    }
}
