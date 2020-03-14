#pragma once

#include "common/macro.h"
#include "common/module.h"

#ifdef COREMODULE
    #define COREMODULE_API PIM_EXPORT
#else
    #define COREMODULE_API PIM_IMPORT
#endif // COREMODULE

PIM_C_BEGIN

typedef struct
{
    void (PIM_CDECL *Init)(int32_t argc, char** argv);
    void (PIM_CDECL *Update)(void);
    void (PIM_CDECL *Shutdown)(void);
} core_module_t;

COREMODULE_API const core_module_t* core_module_export(void);

PIM_C_END
