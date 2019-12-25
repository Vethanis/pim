#include "common/hashstring.h"
#include "containers/hash_set.h"
#include "common/text.h"

#if _DEBUG

namespace HStr
{
    using HashText = Text<64>;

    static constexpr auto HashComparator = OpComparator<HashKey>();
    static HashDict<HashKey, HashText, HashComparator> ms_store;

    cstr Lookup(HashKey key)
    {
        cstr result = 0;
        HashText* pText = ms_store.Get(key);
        if (pText)
        {
            result = pText->value;
        }
        return result;
    }

    void Insert(HashKey key, cstrc value)
    {
        if (!value || !value[0])
        {
            return;
        }

        HashText& dst = ms_store[key];
        if (dst.value[0])
        {
            if (!StrICmp(ARGS(dst.value), value))
            {
                return;
            }
            DBG_INT();
            return;
        }

        StrCpy(ARGS(dst.value), value);
    }

}; // HStr

#endif // _DEBUG
