
#include <sokol/sokol_app.h>
#include "systems/systems.h"

sapp_desc sokol_main(int argc, char** argv)
{
    return (sapp_desc)
    {
        .width=640,
        .height=480,
        .init_cb=systems_init,
        .frame_cb=systems_update,
        .cleanup_cb=systems_shutdown,
        .event_cb=systems_onevent,
    };
}
