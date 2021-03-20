#include "assets/crate.h"
#include "allocator/allocator.h"
#include <string.h>

// ----------------------------------------------------------------------------

enum
{
    kSplitSize = 1024,
    kHeaderSize = (sizeof(Guid) + sizeof(i32) + sizeof(i32)) * kCrateLen,
};

// ----------------------------------------------------------------------------

static i32 crate_find(const Crate *const crate, Guid key)
{
    if (!guid_isnull(key))
    {
        const Guid* pim_noalias ids = crate->ids;
        const i32* pim_noalias sizes = crate->sizes;
        for (i32 i = 0; (i < kCrateLen) && sizes[i]; ++i)
        {
            if (guid_eq(ids[i], key))
            {
                return i;
            }
        }
    }
    return -1;
}

static i32 crate_findempty(const Crate *const crate, i32 size)
{
    i32 chosen = -1;
    i32 chosenDiff = 1 << 30;
    if (size > 0)
    {
        const Guid* pim_noalias ids = crate->ids;
        const i32* pim_noalias sizes = crate->sizes;
        for (i32 i = 0; (i < kCrateLen) && sizes[i]; ++i)
        {
            if (guid_isnull(ids[i]))
            {
                i32 diff = sizes[i] - size;
                if ((diff >= 0) && (diff < chosenDiff))
                {
                    chosen = i;
                    chosenDiff = diff;
                }
            }
        }
    }
    return chosen;
}

static i32 crate_allocate(Crate *const crate, Guid id, i32 size)
{
    ASSERT(size > 0);
    ASSERT(!guid_isnull(id));

    i32 slot = crate_findempty(crate, size);
    if (slot < 0)
    {
        ASSERT(false);
        return -1;
    }

    Guid* const pim_noalias ids = crate->ids;
    i32* const pim_noalias offsets = crate->offsets;
    i32* const pim_noalias sizes = crate->sizes;

    ids[slot] = id;
    ASSERT(offsets[slot] >= kHeaderSize);
    ASSERT(sizes[slot] >= size);

    i32 diff = sizes[slot] - size;
    if (diff >= kSplitSize)
    {
        i32 next = slot + 1;
        if ((next < kCrateLen) && guid_isnull(ids[kCrateLen - 1]))
        {
            for (i32 i = next + 1; i < kCrateLen; ++i)
            {
                ids[i] = ids[i - 1];
                offsets[i] = offsets[i - 1];
                sizes[i] = sizes[i - 1];
            }
            ids[next] = (Guid) { 0 };
            offsets[next] = offsets[slot] + size;
            sizes[next] = diff;
            sizes[slot] = size;
        }
    }

    return slot;
}

static void crate_free(Crate *const crate, i32 slot)
{
    ASSERT(slot >= 0);
    ASSERT(slot < kCrateLen);

    Guid* const pim_noalias ids = crate->ids;
    i32* const pim_noalias offsets = crate->offsets;
    i32* const pim_noalias sizes = crate->sizes;

    ids[slot] = (Guid) { 0 };
    offsets[slot] = 0;

    const i32 prev = slot - 1;
    if ((prev >= 0) && guid_isnull(ids[prev]))
    {
        sizes[prev] += sizes[slot];
        sizes[slot] = 0;
        for (i32 i = slot + 1; (i < kCrateLen) && sizes[i]; ++i)
        {
            ids[i - 1] = ids[i];
            offsets[i - 1] = offsets[i];
            sizes[i - 1] = sizes[i];
        }
        ids[kCrateLen - 1] = (Guid) { 0 };
        offsets[kCrateLen - 1] = 0;
        sizes[kCrateLen - 1] = 0;
        --slot;
    }

    const i32 next = slot + 1;
    if ((next < kCrateLen) && guid_isnull(ids[next]))
    {
        sizes[slot] += sizes[next];
        sizes[next] = 0;
        offsets[next] = 0;
        for (i32 i = next + 1; (i < kCrateLen) && sizes[i]; ++i)
        {
            ids[i - 1] = ids[i];
            offsets[i - 1] = offsets[i];
            sizes[i - 1] = sizes[i];
        }
        ids[kCrateLen - 1] = (Guid) { 0 };
        offsets[kCrateLen - 1] = 0;
        sizes[kCrateLen - 1] = 0;
    }
}

// ----------------------------------------------------------------------------

bool crate_open(Crate *const crate, const char* path)
{
    memset(crate, 0, sizeof(*crate));
    bool exists = true;
    fstr_t file = fstr_open(path, "rb+");
    if (!fstr_isopen(file))
    {
        exists = false;
        file = fstr_open(path, "wb+");
    }
    if (!fstr_isopen(file))
    {
        return false;
    }
    crate->file = file;
    fstr_seek(file, 0);
    if (exists)
    {
        fstr_read(file, crate->ids, sizeof(crate->ids) + sizeof(crate->offsets) + sizeof(crate->sizes));
    }
    else
    {
        crate->offsets[0] = kHeaderSize;
        crate->sizes[0] = 1 << 30;
        fstr_write(file, crate->ids, sizeof(crate->ids) + sizeof(crate->offsets) + sizeof(crate->sizes));
    }
    fstr_seek(file, kHeaderSize);
    return true;
}

bool crate_close(Crate *const crate)
{
    bool wasopen = false;
    if (crate)
    {
        fstr_t file = crate->file;
        if (fstr_isopen(file))
        {
            fstr_seek(file, 0);
            fstr_write(file, crate->ids, sizeof(crate->ids) + sizeof(crate->offsets) + sizeof(crate->sizes));
            fstr_flush(file);
            fstr_close(&file);
            wasopen = true;
        }
        memset(crate, 0, sizeof(*crate));
    }
    return wasopen;
}

bool crate_get(Crate *const crate, Guid id, void* dst, i32 size)
{
    ASSERT(dst);
    ASSERT(size > 0);
    ASSERT(!guid_isnull(id));

    fstr_t file = crate->file;
    ASSERT(fstr_isopen(file));

    i32 slot = crate_find(crate, id);
    if (slot >= 0)
    {
        i32 offset = crate->offsets[slot];
        ASSERT(offset >= kHeaderSize);
        if (size > crate->sizes[slot])
        {
            ASSERT(false);
            return false;
        }
        fstr_seek(file, offset);
        return fstr_read(file, dst, size) == size;
    }
    return false;
}

bool crate_set(Crate *const crate, Guid id, const void* src, i32 size)
{
    ASSERT(src);
    ASSERT(size > 0);
    ASSERT(!guid_isnull(id));

    fstr_t file = crate->file;
    ASSERT(fstr_isopen(file));

    i32 slot = crate_find(crate, id);
    i32 offset = -1;
    if (slot >= 0)
    {
        if (crate->sizes[slot] >= size)
        {
            goto writefile;
        }
        else
        {
            crate_free(crate, slot);
            slot = -1;
        }
    }

    slot = crate_allocate(crate, id, size);
    if (slot < 0)
    {
        ASSERT(false);
        return false;
    }

writefile:
    offset = crate->offsets[slot];
    ASSERT(offset >= kHeaderSize);
    ASSERT(crate->sizes[slot] >= size);
    fstr_seek(file, offset);
    bool wroteAll = fstr_write(file, src, size) == size;
    ASSERT(wroteAll);
    return wroteAll;
}

bool crate_rm(Crate *const crate, Guid id)
{
    i32 slot = crate_find(crate, id);
    if (slot >= 0)
    {
        crate_free(crate, slot);
        return true;
    }
    return false;
}

bool crate_stat(const Crate *const crate, Guid id, i32 *const offsetOut, i32 *const sizeOut)
{
    i32 slot = crate_find(crate, id);
    if (slot >= 0)
    {
        *offsetOut = crate->offsets[slot];
        *sizeOut = crate->sizes[slot];
        ASSERT(*sizeOut > 0);
        ASSERT(*offsetOut >= kHeaderSize);
        return true;
    }
    return false;
}
