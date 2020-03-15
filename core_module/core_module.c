#include "core_module.h"
#include <stdio.h>

static void Init(int32_t argc, char** argv);
static void Update(void);
static void Shutdown(void);

static const core_module_t kModule =
{
    Init,
    Update,
    Shutdown,
};

static void Init(int32_t argc, char** argv)
{
    puts("core_module initialized");
    puts(argv[0]);
}

static void Update(void)
{

}

static void Shutdown(void)
{

}

COREMODULE_API const core_module_t* core_module_export(void)
{
    return &kModule;
}
