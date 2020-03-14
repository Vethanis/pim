#pragma once

// pim code modules:
// implement and export this function in your dll:
// pim_err_t %MODULE_NAME%_export(pimod_t* mod_out);
// where %MODULE_NAME% is the name of your module

#include "common/macro.h"

PIM_C_BEGIN

#include <stdint.h>

// returns address of your function table
// Example:
//   MyModule.h:
//      typedef struct { void(*Init)(void); } MyModule;
//      MY_API const void* your_module_export(void);
//   MyModule.c:
//      static const MyModule kModule = { Init };
//      MY_API const void* your_module_export(void) { return &kModule; }
typedef const void* (PIM_CDECL *pimod_export_t)(void);

const void* PIM_CDECL pimod_load(const char* name);
void PIM_CDECL pimod_unload(const char* name);

PIM_C_END
