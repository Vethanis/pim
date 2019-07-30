#include "systems.h"

#include "audio_system.h"
#include "ctrl_system.h"
#include "entity_system.h"
#include "render_system.h"
#include "time_system.h"

#include <sokol/sokol_app.h>

void systems_init()
{
    timesys_init();
    audiosys_init();
    rendersys_init();
    ctrlsys_init();
    entsys_init();
}

void systems_update()
{
    float dt = timesys_update();
    ctrlsys_update(dt);
    entsys_update(dt);
    audiosys_update(dt);
    rendersys_update(dt);
}

void systems_shutdown()
{
    ctrlsys_shutdown();
    entsys_shutdown();
    rendersys_shutdown();
    audiosys_shutdown();
    timesys_shutdown();
}

void systems_onevent(const sapp_event* evt)
{

}