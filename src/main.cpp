#include <stdio.h>
#include <string.h>

#include <sokol/sokol_app.h>
#include <sokol/sokol_gfx.h>
#include <sokol/sokol_time.h>
#include <sokol/sokol_audio.h>

static void AppInit()
{
    puts("Hello World!");
}

static void AppUpdate()
{

}

static void AppShutdown()
{
    puts("Goodbye World!");
}

static void AppHandleEvent(const sapp_event* evt)
{

}

sapp_desc sokol_main(int argc, char* argv[])
{
    sapp_desc desc;
    memset(&desc, 0, sizeof(desc));
    desc.width = 640;
    desc.height = 480;
    desc.init_cb = AppInit;
    desc.frame_cb = AppUpdate;
    desc.cleanup_cb = AppShutdown;
    desc.event_cb = AppHandleEvent;
    return desc;
}
