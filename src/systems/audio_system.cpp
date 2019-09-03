#include "audio_system.h"

#include <sokol/sokol_audio.h>

namespace AudioSystem
{
    static void AudioMain(float* buffer, int num_frames, int num_channels)
    {
        for (int i = 0; i < num_frames; i++)
        {
            buffer[2 * i + 0] = 0.0f;     // left channel
            buffer[2 * i + 1] = 0.0f;     // right channel
        }
    }

    void Init()
    {
        saudio_desc desc = {};
        desc.num_channels = 2;
        desc.sample_rate = 44100;
        desc.stream_cb = AudioMain;
        saudio_setup(&desc);
    }

    void Update()
    {

    }

    void Shutdown()
    {
        saudio_shutdown();
    }

    void Visualize()
    {

    }
};
