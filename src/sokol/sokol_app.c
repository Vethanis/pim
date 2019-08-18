
#include "common/macro.h"
#include "sokol/sokol_backend.h"

#define SOKOL_ASSERT                DebugAssert
#define SOKOL_WIN32_FORCE_MAIN      1
#define SOKOL_IMPL                  1

#include <sokol/sokol_app.h>
#include <sokol/sokol_gfx.h>
#include <sokol/util/sokol_imgui.h>