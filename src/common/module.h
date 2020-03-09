#pragma once

// pim code modules:
// implement and export this function in your dll:
// pim_err_t %MODULE_NAME%_export(pimod_t* mod_out);
// where %MODULE_NAME% is the name of your module

#include "common/macro.h"

PIM_C_BEGIN

#define _CRT_SECURE_NO_WARNINGS

#include <stdint.h>
#include <string.h>

typedef int32_t pim_err_t;
typedef struct pimod_s pimod_t;

typedef pim_err_t(PIM_CDECL *pimod_export_t)(pimod_t* mod_out);
typedef pim_err_t(PIM_CDECL *pimod_func_t)(void** args, int32_t count);
typedef pim_err_t(PIM_CDECL *pimod_init_t)(const pimod_t* modules, int32_t count);
typedef pim_err_t(PIM_CDECL *pimod_update_t)(void);
typedef pim_err_t(PIM_CDECL *pimod_shutdown_t)(void);

typedef struct pimod_named_func_s
{
    const char* name;
    pimod_func_t func;
} pimod_named_func_t;

#define PIM_NAMED_FUNC(x) { #x, x }

typedef struct pimod_s
{
    const char* name;

    const char* const* imports;
    int32_t import_count;

    const pimod_named_func_t* exports;
    int32_t export_count;

    pimod_init_t init;
    pimod_update_t update;
    pimod_shutdown_t shutdown;
} pimod_t;

pim_err_t PIM_CDECL pimod_load(const char* name, pimod_t* mod_out);
pim_err_t PIM_CDECL pimod_unload(const char* name);

static const pimod_t* pimod_findmod(const pimod_t* modules, int32_t count, const char* name)
{
    for (int32_t i = 0; i < count; ++i)
    {
        if (strcmp(modules[i].name, name) == 0)
        {
            return modules + i;
        }
    }
    return NULL;
}

static pimod_func_t pimod_findfunc(const pimod_named_func_t* funcs, int32_t count, const char* name)
{
    for (int32_t i = 0; i < count; ++i)
    {
        if (strcmp(funcs[i].name, name) == 0)
        {
            return funcs[i].func;
        }
    }
    return NULL;
}

static pimod_func_t pimod_find(const pimod_t* modules, int32_t count, const char* modname, const char* funcname)
{
    const pimod_t* mod = pimod_findmod(modules, count, modname);
    if (mod)
    {
        return pimod_findfunc(mod->exports, mod->export_count, funcname);
    }
    return NULL;
}

PIM_C_END
