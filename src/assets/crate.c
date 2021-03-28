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
    if (!Guid_IsNull(key))
    {
        const Guid* pim_noalias ids = crate->ids;
        const i32* pim_noalias sizes = crate->sizes;
        for (i32 i = 0; (i < kCrateLen) && sizes[i]; ++i)
        {
            if (Guid_Equal(ids[i], key))
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
            if (Guid_IsNull(ids[i]))
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
    ASSERT(!Guid_IsNull(id));

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
        if ((next < kCrateLen) && Guid_IsNull(ids[kCrateLen - 1]))
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
    if ((prev >= 0) && Guid_IsNull(ids[prev]))
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
    if ((next < kCrateLen) && Guid_IsNull(ids[next]))
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

bool Crate_Open(Crate *const crate, const char* path)
{
    memset(crate, 0, sizeof(*crate));
    bool exists = true;
    FStream file = FStream_Open(path, "rb+");
    if (!FStream_IsOpen(file))
    {
        exists = false;
        file = FStream_Open(path, "wb+");
    }
    if (!FStream_IsOpen(file))
    {
        return false;
    }
    crate->file = file;
    FStream_Seek(file, 0);
    if (exists)
    {
        FStream_Read(file, crate->ids, sizeof(crate->ids) + sizeof(crate->offsets) + sizeof(crate->sizes));
    }
    else
    {
        crate->offsets[0] = kHeaderSize;
        crate->sizes[0] = 1 << 30;
        FStream_Write(file, crate->ids, sizeof(crate->ids) + sizeof(crate->offsets) + sizeof(crate->sizes));
    }
    FStream_Seek(file, kHeaderSize);
    return true;
}

bool Crate_Close(Crate *const crate)
{
    bool wasopen = false;
    if (crate)
    {
        FStream file = crate->file;
        if (FStream_IsOpen(file))
        {
            FStream_Seek(file, 0);
            FStream_Write(file, crate->ids, sizeof(crate->ids) + sizeof(crate->offsets) + sizeof(crate->sizes));
            FStream_Flush(file);
            FStream_Close(&file);
            wasopen = true;
        }
        memset(crate, 0, sizeof(*crate));
    }
    return wasopen;
}

bool Crate_Get(Crate *const crate, Guid id, void* dst, i32 size)
{
    ASSERT(dst);
    ASSERT(size > 0);
    ASSERT(!Guid_IsNull(id));

    FStream file = crate->file;
    ASSERT(FStream_IsOpen(file));

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
        FStream_Seek(file, offset);
        return FStream_Read(file, dst, size) == size;
    }
    return false;
}

bool Crate_Set(Crate *const crate, Guid id, const void* src, i32 size)
{
    ASSERT(src);
    ASSERT(size > 0);
    ASSERT(!Guid_IsNull(id));

    FStream file = crate->file;
    ASSERT(FStream_IsOpen(file));

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
    FStream_Seek(file, offset);
    bool wroteAll = FStream_Write(file, src, size) == size;
    ASSERT(wroteAll);
    return wroteAll;
}

bool Crate_Rm(Crate *const crate, Guid id)
{
    i32 slot = crate_find(crate, id);
    if (slot >= 0)
    {
        crate_free(crate, slot);
        return true;
    }
    return false;
}

bool Crate_Stat(const Crate *const crate, Guid id, i32 *const offsetOut, i32 *const sizeOut)
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
