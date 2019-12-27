#if _DEBUG

#include "common/hashstring.h"
#include "common/text.h"
#include "containers/hash_dict.h"

namespace HStr
{
    using HashText = Text<64>;

    static constexpr auto HashComparator = OpComparator<HashKey>();
    static HashDict<HashKey, HashText, HashComparator> ms_store = { Alloc_Pool };

    cstr Lookup(HashKey key)
    {
        HashText* pText = ms_store.Get(key);
        return pText ? pText->value : nullptr;
    }

    void Insert(HashKey key, cstrc value)
    {
        if (!value || !value[0])
        {
            return;
        }

        HashText& dst = ms_store[key];
        ASSERT(!dst.value[0] || !StrICmp(ARGS(dst.value), value));
        StrCpy(ARGS(dst.value), value);
    }

}; // HStr

#endif // _DEBUG
