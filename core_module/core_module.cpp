#include "core_module.h"
#include <stdio.h>

static const pimod_t* s_modules;
static int32_t s_modcount;

static pim_err_t Init(const pimod_t* modules, int32_t count)
{
    s_modules = modules;
    s_modcount = count;

    puts("core_module Initialized");

    return PIM_ERR_OK;
}

static pim_err_t Update(void)
{
    return PIM_ERR_OK;
}

static pim_err_t Shutdown(void)
{
    s_modules = NULL;
    s_modcount = 0;
    return PIM_ERR_OK;
}

static pim_err_t ExampleFn(void** args, int32_t count)
{
    return PIM_ERR_OK;
}

static constexpr pimod_named_func_t kExports[] =
{
    PIM_NAMED_FUNC(ExampleFn),
};

static constexpr pimod_t s_module =
{
    "core_module",
    0, 0,
    ARGS(kExports),
    Init,
    Update,
    Shutdown,
};

COREMODULE_API pim_err_t core_module_export(pimod_t* mod_out)
{
    *mod_out = s_module;
    return 0;
}
