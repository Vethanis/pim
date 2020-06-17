#include "containers/sset.h"
#include "common/fnv1a.h"
#include "containers/hash_util.h"
#include "common/stringutil.h"

u32 sset_hash(const char* key)
{
    if (key)
    {
        return hashutil_create_hash(HashStr(key));
    }
    return 0u;
}

i32 sset_find(const char** keys, const u32* hashes, i32 width, const char* key)
{
    if (!key || !key[0])
    {
        return -1;
    }

    const u32 keyHash = sset_hash(key);
    const u32 mask = width - 1u;

    u32 j = keyHash;
    while (width--)
    {
        j &= mask;
        const u32 jHash = hashes[j];
        if (hashutil_empty(jHash))
        {
            break;
        }
        if (jHash == keyHash)
        {
            if (StrCmp(key, PIM_PATH, keys[j]) == 0)
            {
                return (i32)j;
            }
        }
        ++j;
    }

    return -1;
}
