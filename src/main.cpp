#include <stdio.h>
#include <string.h>

#include <sokol/sokol_app.h>
#include <sokol/sokol_gfx.h>
#include <sokol/sokol_time.h>
#include <sokol/sokol_audio.h>

static void AudioMain(float* buffer, int num_frames, int num_channels)
{
    for (int i = 0; i < num_frames; i++)
    {
        buffer[2*i + 0] = 0.0f;     // left channel
        buffer[2*i + 1] = 0.0f;     // right channel
    }
}

static void AppInit()
{
    stm_setup();
    {
        saudio_desc desc;
        memset(&desc, 0, sizeof(desc));
        desc.num_channels = 2;
        desc.sample_rate = 44100;
        desc.stream_cb = AudioMain;
        saudio_setup(&desc);
    }

    sg_desc desc;
    memset(&desc, 0, sizeof(desc));
    desc.mtl_device = sapp_metal_get_device();
    desc.mtl_drawable_cb = sapp_metal_get_drawable;
    desc.mtl_renderpass_descriptor_cb = sapp_metal_get_renderpass_descriptor;
    desc.d3d11_device = sapp_d3d11_get_device();
    desc.d3d11_device_context = sapp_d3d11_get_device_context();
    desc.d3d11_render_target_view_cb = sapp_d3d11_get_render_target_view;
    desc.d3d11_depth_stencil_view_cb = sapp_d3d11_get_depth_stencil_view;

    sg_setup(&desc);
}

static void AppUpdate()
{
    sg_pass_action action;
    memset(&action, 0, sizeof(action));
    sg_begin_default_pass(&action, sapp_width(), sapp_height());

    sg_end_pass();
    sg_commit();
}

static void AppShutdown()
{
    sg_shutdown();
    saudio_shutdown();
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
