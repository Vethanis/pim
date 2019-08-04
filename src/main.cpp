
#include <sokol/sokol_app.h>
#include "systems/systems.h"

sapp_desc sokol_main(int argc, char** argv)
{
    sapp_desc desc = {0};
    desc.width = 640;
    desc.height = 480;
    desc.init_cb = Systems::Init;
    desc.frame_cb = Systems::Update;
    desc.cleanup_cb = Systems::Shutdown;
    desc.event_cb = Systems::OnEvent;
    return desc;
}
