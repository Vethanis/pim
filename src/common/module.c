#include "common/module.h"
#include <string.h>

static const uint64_t kFnv64Offset = 0xcbf29ce484222325;
static const uint64_t kFnv64Prime = 0x100000001b3;

uint64_t pim_hash64(const void* bytes, int32_t count, uint64_t seed)
{
    uint64_t x = 0;
    if (bytes)
    {
        x = seed;
        const uint8_t* octets = (const uint8_t*)bytes;
        for (int32_t i = 0; i < count; ++i)
        {
            x = (x ^ octets[i]) * kFnv64Prime;
        }
    }
    return x;
}

pim_guid_t pim_guid_memory(const void* bytes, int32_t count)
{
    pim_guid_t guid = { 0, 0 };
    if (bytes)
    {
        guid.lo = pim_hash64(bytes, count, kFnv64Offset);
        guid.hi = pim_hash64(bytes, count, guid.lo);
        guid.lo = guid.lo ? guid.lo : 1;
        guid.hi = guid.hi ? guid.hi : 1;
    }
    return guid;
}

pim_guid_t pim_guid_string(const char* str)
{
    pim_guid_t guid = { 0, 0 };
    if (str)
    {
        guid = pim_guid_memory(str, (int32_t)strlen(str));
    }
    return guid;
}

#if PLAT_WINDOWS

#include <Windows.h>

pim_err_t PIM_CDECL pimod_load(const char* name, pimod_t* mod_out)
{
    if (!name || !mod_out)
    {
        return PIM_ERR_ARG_NULL;
    }
    memset(mod_out, 0, sizeof(pimod_t));
    HMODULE hmod = LoadLibraryA(name);
    if (!hmod)
    {
        return PIM_ERR_NOT_FOUND;
    }
    FARPROC proc = GetProcAddress(hmod, "pimod_export");
    if (!proc)
    {
        FreeLibrary(hmod);
        return PIM_ERR_NOT_FOUND;
    }
    pimod_export_t exporter = (pimod_export_t)proc;
    pim_err_t status = exporter(mod_out);
    if (status != PIM_ERR_OK)
    {
        FreeLibrary(hmod);
    }
    return status;
}

#endif // PLAT_WINDOWS
