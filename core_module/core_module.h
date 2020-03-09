#include "common/macro.h"
#include "common/module.h"

#ifdef COREMODULE
    #define COREMODULE_API PIM_EXPORT
#else
    #define COREMODULE_API PIM_IMPORT
#endif // COREMODULE

PIM_C_BEGIN

COREMODULE_API pim_err_t core_module_export(pimod_t* mod_out);

PIM_C_END
