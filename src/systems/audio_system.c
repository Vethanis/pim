#include "audio_system.h"

#include <string.h>
#include <sokol/sokol_audio.h>

static void audio_main(float* buffer, int num_frames, int num_channels)
{
    for (int i = 0; i < num_frames; i++)
    {
        buffer[2*i + 0] = 0.0f;     // left channel
        buffer[2*i + 1] = 0.0f;     // right channel
    }
}

void audiosys_init()
{
    saudio_setup(&(saudio_desc)
    {
        .num_channels = 2,
        .sample_rate = 44100,
        .stream_cb = audio_main,
    });
}

void audiosys_update(float dt)
{

}

void audiosys_shutdown()
{
    saudio_shutdown();
}