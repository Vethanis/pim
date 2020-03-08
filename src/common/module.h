#pragma once

#include "common/macro.h"
#include <stdint.h>

PIM_C_BEGIN

typedef struct pim_guid_s
{
    uint64_t lo;
    uint64_t hi;
} pim_guid_t;

uint64_t pim_hash64(const void* bytes, int32_t count, uint64_t seed);
pim_guid_t pim_guid_memory(const void* bytes, int32_t count);
pim_guid_t pim_guid_string(const char* str);

typedef int32_t pim_err_t;

typedef pim_err_t(PIM_CDECL *pimod_func_t)(void** args, int32_t count);
typedef pim_err_t(PIM_CDECL *pimod_init_t)(const struct pimod_s* modules, int32_t count);
typedef pim_err_t(PIM_CDECL *pimod_update_t)(void);
typedef pim_err_t(PIM_CDECL *pimod_shutdown_t)(void);

typedef struct pimod_s
{
    const char* name;

    const char** import_names;
    int32_t import_count;

    const char** export_names;
    const pimod_func_t* export_fns;
    int32_t export_count;

    pimod_init_t init;
    pimod_update_t update;
    pimod_shutdown_t shutdown;
} pimod_t;

// implement and export this function in your dll:
// pim_err_t pimod_export(pimod_t* mod_out);
typedef pim_err_t(PIM_CDECL *pimod_export_t)(pimod_t* mod_out);

pim_err_t PIM_CDECL pimod_load(const char* name, pimod_t* mod_out);

PIM_C_END
