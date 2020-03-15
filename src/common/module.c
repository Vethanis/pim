#include "common/module.h"
#include "common/hashstring.h"
#include <string.h>

#define kMaxModules 256

static const void* ms_modules[kMaxModules];
static uint32_t ms_hashes[kMaxModules];
static int32_t ms_count;

static int32_t pimod_find(uint32_t hash)
{
    const int32_t count = ms_count;
    const uint32_t* const hashes = ms_hashes;
    for (int32_t i = 0; i < count; ++i)
    {
        if (hashes[i] == hash)
        {
            return i;
        }
    }
    return -1;
}

int32_t PIM_CDECL pimod_register(const char* name, void* dst, int32_t bytes)
{
    ASSERT(name);
    ASSERT(dst);

    const void* pModule = 0;
    const uint32_t hash = HashStr(name);
    const int32_t i = pimod_find(hash);
    if (i != -1)
    {
        pModule = ms_modules[i];
    }
    else
    {
        const int32_t back = ms_count++;
        const int32_t count = back + 1;
        ASSERT(count <= kMaxModules);

        pModule = pimod_load(name);
        ms_hashes[back] = hash;
        ms_modules[back] = pModule;
    }

    if (pModule)
    {
        memcpy(dst, pModule, bytes);
    }

    return pModule != 0;
}

int32_t PIM_CDECL pimod_get(const char* name, void* dst, int32_t bytes)
{
    ASSERT(name);
    ASSERT(dst);
    const void* pModule = 0;
    const uint32_t hash = HashStr(name);
    const int32_t i = pimod_find(hash);
    if (i != -1)
    {
        pModule = ms_modules[i];
        if (pModule)
        {
            memcpy(dst, pModule, bytes);
        }
    }
    return pModule != 0;
}

#if PLAT_WINDOWS

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <Windows.h>

const void* PIM_CDECL pimod_load(const char* name)
{
    if (!name)
    {
        return 0;
    }

    HMODULE hmod = LoadLibrary(name);
    if (!hmod)
    {
        return 0;
    }

    char procname[256];
    sprintf_s(ARGS(procname), "%s_export", name);
    procname[NELEM(procname) - 1] = 0;

    FARPROC proc = GetProcAddress(hmod, procname);
    if (!proc)
    {
        FreeLibrary(hmod);
        return 0;
    }

    pimod_export_t exporter = (pimod_export_t)proc;
    const void* pModule = exporter();
    if (!pModule)
    {
        FreeLibrary(hmod);
        return 0;
    }

    return pModule;
}

void PIM_CDECL pimod_unload(const char* name)
{
    if (name)
    {
        HMODULE hmod = GetModuleHandle(name);
        if (hmod)
        {
            FreeLibrary(hmod);
        }
    }
}

#endif // PLAT_WINDOWS
