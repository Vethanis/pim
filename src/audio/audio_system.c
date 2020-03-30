#include "audio_system.h"
#include <sokol/sokol_audio.h>

static void audio_main(float* buffer, int32_t num_frames, int32_t num_channels)
{
    for (int32_t i = 0; i < num_frames; ++i)
    {
        *buffer++ = 0.0f;
        *buffer++ = 0.0f;
    }
}

void audio_sys_init(void)
{
    saudio_desc desc =
    {
        .num_channels = 2,
        .sample_rate = 44100,
        .stream_cb = audio_main,
    };
    saudio_setup(&desc);
}

void audio_sys_update(void)
{

}

void audio_sys_shutdown(void)
{
    saudio_shutdown();
}
