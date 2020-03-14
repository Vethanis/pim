#include "common/module.h"

#if PLAT_WINDOWS

#define _CRT_SECURE_NO_WARNINGS

#include <string.h>
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
