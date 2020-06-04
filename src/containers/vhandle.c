#include "containers/vhandle.h"
#include "allocator/allocator.h"
#include "common/atomics.h"
#include <string.h>

// guards against memory scribbling on free
#define kStartVersion 1855542631u

static u64 ms_version;

vhandle_t vhandle_new(const void* src, i32 sizeOf)
{
    ASSERT(src);
    ASSERT(sizeOf > 0);

    vhandle_t hdl = { 0 };
    u64* pVersion = perm_calloc(sizeOf + 16);
    hdl.handle = pVersion;
    hdl.version = kStartVersion + fetch_add_u64(&ms_version, 4, MO_Relaxed);
    *pVersion = hdl.version;
    memcpy(pVersion + 2, src, sizeOf);

    return hdl;
}

bool vhandle_del(vhandle_t hdl, void* dst, i32 sizeOf)
{
    ASSERT(dst);
    ASSERT(sizeOf > 0);

    u64* pVersion = hdl.handle;
    if (pVersion && cmpex_u64(pVersion, &hdl.version, hdl.version + 1, MO_Release))
    {
        memcpy(dst, pVersion + 2, sizeOf);
        pim_free(pVersion);
        return true;
    }
    return false;
}

bool vhandle_get(vhandle_t hdl, void* dst, i32 sizeOf)
{
    ASSERT(dst);
    ASSERT(sizeOf > 0);

    if (hdl.handle)
    {
        const u64* pVersion = hdl.handle;
        if (load_u64(pVersion, MO_Relaxed) == hdl.version)
        {
            memcpy(dst, pVersion + 2, sizeOf);
            return true;
        }
    }
    return false;
}
