#include "common/module.h"

#if PLAT_WINDOWS

#include <string.h>
#include <stdio.h>
#include <Windows.h>

pim_err_t PIM_CDECL pimod_load(const char* name, pimod_t* mod_out)
{
    if (!name || !mod_out)
    {
        return PIM_ERR_ARG_NULL;
    }

    memset(mod_out, 0, sizeof(pimod_t));
    HMODULE hmod = LoadLibrary(name);
    if (!hmod)
    {
        return PIM_ERR_NOT_FOUND;
    }

    char procname[256];
    sprintf_s(ARGS(procname), "%s_export", name);
    procname[NELEM(procname) - 1] = 0;

    FARPROC proc = GetProcAddress(hmod, procname);
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

pim_err_t PIM_CDECL pimod_unload(const char* name)
{
    if (!name)
    {
        return PIM_ERR_ARG_NULL;
    }

    HMODULE hmod = GetModuleHandle(name);
    if (!hmod)
    {
        return PIM_ERR_NOT_FOUND;
    }

    if (FreeLibrary(hmod))
    {
        return PIM_ERR_OK;
    }

    return PIM_ERR_INTERNAL;
}

#endif // PLAT_WINDOWS
